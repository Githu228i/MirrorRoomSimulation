#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "changeorconnectdialog.h"
#include "changepointdialog.h"
#include "beamangledialog.h"

#include <QPen>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTime>
#include <QVariantAnimation>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(2000,1300);

    // ---- Служебное окно справа (чат системы) ----
    ui->chatScrollArea->setWidgetResizable(true);
    ui->chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->chatScrollArea->setStyleSheet(R"(
        QScrollArea {
            background-color: #202124;
            border: 1px solid #3c4043;
        }

        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 6px 2px 6px 0px;
        }

        QScrollBar::handle:vertical {
            background: #5f6368;
            border-radius: 4px;
            min-height: 30px;
        }

        QScrollBar::handle:vertical:hover {
            background: #80868b;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
        }
    )");

    QWidget *chatContent = new QWidget;
    chatContent->setObjectName("chatContent");
    chatContent->setStyleSheet("#chatContent { background: transparent; }");

    chatLayout = new QVBoxLayout(chatContent);
    chatLayout->setContentsMargins(10, 10, 10, 10);
    chatLayout->setSpacing(8);
    chatLayout->addStretch();            // прижимает сообщения к верху

    ui->chatScrollArea->setWidget(chatContent);
    chatContent->setAutoFillBackground(false);

    // Клики по окошку не должны попадать в mousePressEvent окна
    // (иначе в области чата рисовались бы точки полигона)
    ui->chatScrollArea->setAttribute(Qt::WA_NoMousePropagation);
    ui->chatScrollArea->viewport()->setAttribute(Qt::WA_NoMousePropagation);

    // Новое сообщение появляется сверху - прокрутка к самому верху
    QScrollBar *chatBar = ui->chatScrollArea->verticalScrollBar();
    connect(chatBar, &QScrollBar::rangeChanged, this,
            [chatBar](int, int) { chatBar->setValue(0); });

    // Анимация одного отражения: отрезок растёт от 0 до полной длины
    reflectAnim_ = new QVariantAnimation(this);
    reflectAnim_->setStartValue(0.0);
    reflectAnim_->setEndValue(1.0);
    reflectAnim_->setEasingCurve(QEasingCurve::Linear);

    connect(reflectAnim_, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &v) {
                animProgress_ = v.toDouble();
                update();
            });
    connect(reflectAnim_, &QVariantAnimation::finished,
            this, &MainWindow::FinishReflection);

    // Сообщения от FrontEndOperators (ошибки ввода, отражения и т.д.) идут в чат
    polygonCreator.notify = [this](NoteLevel level, const QString &text) {
        MsgType t = MsgType::Info;
        if (level == NoteLevel::Warning)
            t = MsgType::Warning;
        else if (level == NoteLevel::Error)
            t = MsgType::Error;
        if (bufferNotes_)
            pendingNotes_.push_back({t, text});   // покажем в конце анимации
        else
            AddMessage(text, t);
    };

    AddMessage(
        "Режим редактирования комнаты. Кликайте, чтобы ставить точки "
        "(до 9 сторон). Клик по вершине - изменить или соединить её. "
        "«Начать симуляцию» замкнёт комнату.");

    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::FirstButClicked);

    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::SecondButClicked);

    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::ThirdButClicked);

    ui->pushButton_3->setText("Следующее отражение");

    // Кнопка 4 пока без действия, но во время анимации её нажатия тоже игнорируются
    connect(ui->pushButton_4, &QPushButton::clicked,
            this, [this]() { IgnoreDuringAnimation(); });

    ui->pushButton->setText("Откатить точку");
    ui->pushButton_2->setText("Начать симуляцию");
    ui->pushButton_3->setVisible(false);
    ui->pushButton_4->setVisible(false);

    UpdateButtons();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Доступность кнопок (недоступные приглушаются):
//  - в режиме выбора точки луча недоступны кнопки 2, 3, 4;
//  - если луч зациклился, недоступны кнопки 3 и 4;
//  - во время анимации все кнопки приглушены, но остаются нажимаемыми:
//    нажатие игнорируется с сообщением в чат.
void MainWindow::UpdateButtons()
{
    const bool picking = !Redacting && Beaming;
    const bool looped = polygonCreator.looped_;
    const bool busy = animating_;

    // enabled - принимает ли кнопка нажатия, dim - приглушена ли она
    auto setState = [](QPushButton *b, bool enabled, bool dim)
    {
        b->setEnabled(enabled);

        if (dim)
        {
            auto *effect = new QGraphicsOpacityEffect(b);
            effect->setOpacity(0.35);
            b->setGraphicsEffect(effect);   // старый эффект Qt удаляет сам
        }
        else
        {
            b->setGraphicsEffect(nullptr);
        }
    };

    setState(ui->pushButton,   true,                    busy);
    setState(ui->pushButton_2, !picking,                picking || busy);
    setState(ui->pushButton_3, !picking && !looped,     picking || looped || busy);
    setState(ui->pushButton_4, !picking && !looped,     picking || looped || busy);
}

// Служебное окно: добавляет сообщение-"пузырь" сверху, старые сдвигаются вниз
void MainWindow::AddMessage(const QString &text, MsgType type)
{
    QString bg;
    QString tag;

    switch (type)
    {
    case MsgType::Error:
        bg = "#5c2b2e";
        tag = "Ошибка";
        break;
    case MsgType::Warning:
        bg = "#5c4b1f";
        tag = "Внимание";
        break;
    default:
        bg = "#303134";
        tag = "Система";
        break;
    }

    QString body = text.toHtmlEscaped();
    body.replace("\n", "<br>");

    auto *bubble = new QLabel(
        QString("<span style='color:#9aa0a6; font-size:12px;'>%1 · %2</span><br>%3")
            .arg(tag, QTime::currentTime().toString("HH:mm:ss"), body)
        );

    bubble->setTextFormat(Qt::RichText);
    bubble->setWordWrap(true);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setStyleSheet(QString(
                              "QLabel {"
                              " background-color: %1;"
                              " color: white;"
                              " border-radius: 12px;"
                              " padding: 8px 12px;"
                              " font-size: 14px;"
                              "}").arg(bg));

    // новое сообщение - в самый верх, старые уходят вниз
    // (растяжка в конце прижимает все сообщения к верху)
    chatLayout->insertWidget(0, bubble);
}

void MainWindow::FirstButClicked() {
    if (IgnoreDuringAnimation())
        return;

    if (Redacting) {
        polygonCreator.DeleteLast();
        update();

    } else {
        Redacting = true;
        ui->pushButton->setText("Откатить точку");
        ui->pushButton_2->setText("Начать симуляцию");
        ui->pushButton_3->setVisible(false);
        ui->pushButton_4->setVisible(false);
        const bool hadPath = polygonCreator.history_.size() > 1;
        polygonCreator.ResetTrace();
        UpdateButtons();
        update();   // сразу перерисовать без линий отражений

        AddMessage(hadPath ? "Режим редактирования комнаты. Путь луча сброшен."
                           : "Режим редактирования комнаты.");
        AddMessage(
            "Кликните по вершине, чтобы изменить её координаты и тип зеркала, "
            "которое из неё выходит. «Откатить точку» уберёт замыкающую стену "
            "и разомкнёт комнату. «Начать симуляцию» вернёт к выбору луча.");
        if (polygonCreator.prototype.isBeamSet_)
            AddMessage("Источник луча сохранится, пока вы не измените или не удалите его стену.");
    }
}

void MainWindow::SecondButClicked() {
    if (IgnoreDuringAnimation())
        return;

    if (Redacting) {
        auto &poly = polygonCreator.prototype;

        if (poly.sides_.size() <= 2) {
            AddMessage(
                "Нельзя начать симуляцию: слишком мало стен "
                "(нужно больше двух стен до замыкания). Поставьте ещё точки "
                "или кликните по первой точке и выберите «Подключить».",
                MsgType::Error);
            return;
        }

        // Уже замкнутый полигон повторно не замыкаем
        if (!poly.isCycled_)
            polygonCreator.Binding();
        update();

        // Замкнуть не удалось (причина уже в чате)
        if (!poly.isCycled_)
            return;

        if (!poly.Check()) {
            AddMessage(
                "Комната некорректна: стены пересекаются или касаются "
                "друг друга. Исправьте точки или откатите последнюю.",
                MsgType::Error);
            return;
        }

        Redacting = false;
        ui->pushButton->setText("Редактировать комнату");
        ui->pushButton_2->setText("Переставить источник света");
        ui->pushButton_3->setVisible(true);
        ui->pushButton_4->setVisible(true);

        // форма комнаты могла измениться - пересчитываем направление луча
        if (poly.isBeamSet_)
            poly.SetBeamAngle(poly.beamWallAngle_);
        polygonCreator.ResetTrace();

        // если луч пережил редактирование, выбирать заново не нужно
        Beaming = !poly.isBeamSet_;
        UpdateButtons();

        // Вершины комнаты и типы стен
        QString desc = QString("Симуляция начата. Комната замкнута, сторон: %1.")
                           .arg(static_cast<int>(poly.sides_.size()));
        for (size_t i = 0; i < poly.sides_.size(); ++i)
        {
            const Mirror &m = *poly.sides_[i];

            QString kind = "плоская";
            if (dynamic_cast<const ConcaveMirror *>(&m))
                kind = QString("вогнутая, R = %1").arg(m.getRadius(), 0, 'f', 1);
            else if (dynamic_cast<const ConvexMirror *>(&m))
                kind = QString("выпуклая, R = %1").arg(m.getRadius(), 0, 'f', 1);

            desc += QString("\nТочка %1: (%2; %3), стена %1 - %4")
                        .arg(static_cast<int>(i) + 1)
                        .arg(m.getStart().getX(), 0, 'f', 1)
                        .arg(m.getStart().getY(), 0, 'f', 1)
                        .arg(kind);
        }
        AddMessage(desc);

        if (Beaming)
        {
            AddMessage("Кликните по стене, откуда должен выйти луч.");
        }
        else
        {
            AddMessage(QString("Источник луча сохранён: (%1; %2) на стене %3, угол к стене %4°.")
                           .arg(poly.beam_.getX(), 0, 'f', 1)
                           .arg(poly.beam_.getY(), 0, 'f', 1)
                           .arg(poly.beamWall_ + 1)
                           .arg(poly.beamWallAngle_, 0, 'f', 1));
        }
    } else {
        // Новый источник: старый луч удаляем сразу, ещё до выбора нового
        polygonCreator.prototype.isBeamSet_ = false;
        polygonCreator.ResetTrace();
        Beaming = true;
        UpdateButtons();
        update();
        AddMessage("Источник луча удалён. Кликните по стене, откуда должен выйти новый луч.");
    }
}

void MainWindow::ThirdButClicked() {
    if (IgnoreDuringAnimation())
        return;

    if (Redacting || Beaming || !polygonCreator.prototype.isBeamSet_)
        return;

    // Сообщения о шаге (точка отражения, зацикливание) покажем в конце анимации
    pendingNotes_.clear();
    bufferNotes_ = true;

    if (!polygonCreator.NextReflection())
    {
        // Шаг не получился: сообщение об ошибке показываем сразу
        FlushPendingNotes();
        return;
    }

    // Точка отражения посчитана, теперь отрезок до неё "дорисовывается" анимацией
    animating_ = true;
    animProgress_ = 0.0;
    UpdateButtons();   // на время анимации кнопки тускнеют
    reflectAnim_->setDuration(reflectionAnimMs_);
    reflectAnim_->start();
    update();
}

// Если идёт анимация - сообщает об этом в чат, и вызывающий код должен выйти
bool MainWindow::IgnoreDuringAnimation()
{
    if (!animating_)
        return false;

    AddMessage(
        "Нельзя редактировать, пока идёт симуляция: "
        "дождитесь конца анимации отражения.",
        MsgType::Warning);
    return true;
}

void MainWindow::FinishReflection()
{
    animating_ = false;
    animProgress_ = 1.0;

    FlushPendingNotes();
    UpdateButtons();   // если луч зациклился - кнопки 3 и 4 гаснут
    update();
}

void MainWindow::FlushPendingNotes()
{
    bufferNotes_ = false;

    for (const auto &note : pendingNotes_)
        AddMessage(note.second, note.first);

    pendingNotes_.clear();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (IgnoreDuringAnimation())
        return;

    int x = event->position().x();
    int y = event->position().y();

    if (x > Limits::MaxX || y > Limits::MaxY)
    {
        AddMessage(
            "Клик вне рабочей области (допустимо: x от 0 до 1580, y от 0 до 1000).",
            MsgType::Warning);
        return;
    }
    // Клик по точке источника - изменить угол (в любом режиме)
    if (polygonCreator.prototype.isBeamSet_)
    {
        const Beam &b = polygonCreator.prototype.beam_;
        double bx = x - b.getX();
        double by = y - b.getY();

        if (bx * bx + by * by <= 25)
        {
            BeamAngleDialog angleDialog(
                this,
                polygonCreator.prototype.beamWallAngle_
                );
            angleDialog.move(
                this->geometry().center()
                - angleDialog.rect().center()
                );

            if (angleDialog.exec() == QDialog::Accepted)
            {
                polygonCreator.prototype.SetBeamAngle(angleDialog.getAngle());
                polygonCreator.ResetTrace();
                UpdateButtons();   // новый угол - зацикленность сброшена
                update();

                AddMessage(QString("Угол луча изменён: %1° к стене %2.")
                               .arg(angleDialog.getAngle(), 0, 'f', 1)
                               .arg(polygonCreator.prototype.beamWall_ + 1));
            }
            return;
        }
    }

    if (!Redacting && !Beaming)
    {
        AddMessage(
            "Сейчас идёт симуляция: точки ставить нельзя. "
            "«Переставить источник света» - выбрать другой луч, "
            "«Редактировать комнату» - править стены.",
            MsgType::Warning);
        return;
    }
    if (!Redacting && Beaming)
    {
        BeamPlaceResult res =
            polygonCreator.prototype.PlaceBeam(Points(x, y), 6.0, 10.0);

        if (res == BeamPlaceResult::NoWall)
        {
            AddMessage(
                "Невозможно поставить луч: рядом нет стены. "
                "Кликните точно по линии зеркала (не дальше 6 пикселей).",
                MsgType::Error);
            return;
        }

        if (res == BeamPlaceResult::TooClose)
        {
            AddMessage(
                "Луч нельзя ставить так близко к вершине "
                "(ближе 10 пикселей). Выберите точку подальше.",
                MsgType::Error);
            return;
        }

        repaint();   // показать точку, пока открыто окно выбора угла

        BeamAngleDialog angleDialog(this);
        angleDialog.move(
            this->geometry().center()
            - angleDialog.rect().center()
            );

        if (angleDialog.exec() == QDialog::Accepted)
        {
            auto &poly = polygonCreator.prototype;

            poly.SetBeamAngle(angleDialog.getAngle());
            polygonCreator.ResetTrace();
            Beaming = false;
            UpdateButtons();

            AddMessage(QString("Источник луча: (%1; %2) на стене %3, угол к стене %4°.")
                           .arg(poly.beam_.getX(), 0, 'f', 1)
                           .arg(poly.beam_.getY(), 0, 'f', 1)
                           .arg(poly.beamWall_ + 1)
                           .arg(angleDialog.getAngle(), 0, 'f', 1));
        }
        else
        {
            polygonCreator.prototype.isBeamSet_ = false;
            AddMessage("Выбор угла отменён: источник луча не поставлен.",
                       MsgType::Warning);
        }
        update();
        return;
    }
    const auto &temp = polygonCreator.prototype.sides_;

    // =========================================================
    // СУЩЕСТВУЮЩИЕ ТОЧКИ — НАЧАЛА ЗЕРКАЛ
    // =========================================================

    for (int i = 0; i < temp.size(); ++i)
    {
        double dx =
            x - temp[i]->getStart().getX();

        double dy =
            y - temp[i]->getStart().getY();

        // Попали точно в точку
        if (dx * dx + dy * dy <= 25)
        {
            // -------------------------------------------------
            // ПОЛИГОН УЖЕ ЗАМКНУТ
            // Сразу изменение
            // -------------------------------------------------

            if (polygonCreator.prototype.isCycled_)
            {
                ChangePointDialog changeDialog(
                    i + 1,
                    temp[i]->getStart(),
                    temp[i].get(),
                    false,
                    this
                    );

                changeDialog.move(
                    this->geometry().center()
                    - changeDialog.rect().center()
                    );

                if (changeDialog.exec() == QDialog::Accepted)
                {
                    polygonCreator.ChangeMirror(
                        i,
                        changeDialog.getData()
                        );

                    update();
                }

                return;
            }

            // -------------------------------------------------
            // ПОЛИГОН НЕ ЗАМКНУТ
            // Показываем ChangeOrConnectDialog
            // -------------------------------------------------

            ChangeOrConnectDialog dialog(this);

            dialog.move(
                this->geometry().center()
                - dialog.rect().center()
                );

            const int choice = dialog.exec();

            if (choice == QDialog::Rejected)
            {
                // Esc или закрытие окна - ничего не делаем
                AddMessage("Действие с точкой отменено.");
                return;
            }

            // Accepted - "Изменить", любой другой код (2) - "Подключить"
            if (choice == QDialog::Accepted)
            {
                // Нажали "Изменить"

                ChangePointDialog changeDialog(
                    i + 1,
                    temp[i]->getStart(),
                    temp[i].get(),
                    false,
                    this
                    );

                changeDialog.move(
                    this->geometry().center()
                    - changeDialog.rect().center()
                    );

                if (changeDialog.exec() == QDialog::Accepted)
                {
                    polygonCreator.ChangeMirror(
                        i,
                        changeDialog.getData()
                        );

                    update();
                }

                return;
            }
            else
            {
                // Нажали "Подключить"

                // Последнее зеркало нельзя соединять
                // само с собой
                if (i == temp.size() - 1)
                {
                    AddMessage("Нельзя соединять зеркало с собой же!", MsgType::Error);

                    return;
                }

                polygonCreator.ConnectPoints(i);

                update();

                return;
            }
        }

        // Слишком близко к существующей точке
        if (dx * dx + dy * dy <= 100)
        {
            AddMessage("Нельзя поставить точку: она слишком близко к другой точке (ближе 10 пикселей).", MsgType::Error);

            return;
        }
    }

    // =========================================================
    // ПОСЛЕДНЯЯ ТОЧКА НЕЗАМКНУТОГО ПОЛИГОНА
    // =========================================================

    if (!temp.empty()
        && !polygonCreator.prototype.isCycled_)
    {
        double dx =
            x - temp.back()->getEnd().getX();

        double dy =
            y - temp.back()->getEnd().getY();

        // Попали в последнюю точку
        if (dx * dx + dy * dy <= 25)
        {
            // Последнюю точку нельзя подключить,
            // поэтому сразу открываем изменение

            ChangePointDialog changeDialog(
                temp.size() + 1,
                temp.back()->getEnd(),
                temp.back().get(),
                true,
                this
                );

            changeDialog.move(
                this->geometry().center()
                - changeDialog.rect().center()
                );

            if (changeDialog.exec() == QDialog::Accepted)
            {
                polygonCreator.ChangeMirror(
                    temp.size(),
                    changeDialog.getData()
                    );

                update();
            }

            return;
        }

        // Слишком близко к последней точке
        if (dx * dx + dy * dy <= 100)
        {
            AddMessage("Нельзя поставить точку: она слишком близко к другой точке (ближе 10 пикселей).", MsgType::Error);

            return;
        }
    }

    // =========================================================
    // ЕСЛИ НЕ ПОПАЛИ НИ В ОДНУ ТОЧКУ
    // СОЗДАЁМ НОВУЮ
    // =========================================================

    if (polygonCreator.prototype.isCycled_)
    {
        AddMessage(
            "Комната замкнута: новые точки ставить нельзя. "
            "Нажмите «Откатить точку», чтобы разомкнуть комнату, "
            "или кликните по вершине, чтобы изменить её.",
            MsgType::Error);
        return;
    }

    // Если существует только первая точка
    if (temp.empty()
        && polygonCreator.prototype.isExist())
    {
        double dx =
            x - polygonCreator.prototype.first.getX();

        double dy =
            y - polygonCreator.prototype.first.getY();

        if (dx * dx + dy * dy <= 100)
        {
            AddMessage("Нельзя поставить точку: она слишком близко к другой точке (ближе 10 пикселей).", MsgType::Error);

            return;
        }
    }

    polygonCreator.CreatePoint(x, y);

    update();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QPen pen;
    pen.setWidth(Style::WallWidth);

    painter.setPen(pen);
    painter.setBrush(Qt::black);

    const auto &temp = polygonCreator.prototype.sides_;

    if (!polygonCreator.prototype.isExist())
        return;

    painter.drawEllipse(
        QPointF(polygonCreator.prototype.first.getX(),
                polygonCreator.prototype.first.getY()),
        Style::PointRadius,
        Style::PointRadius
        );

    if (!temp.empty())
    {
        painter.drawLine(
            polygonCreator.prototype.first.getX(),
            polygonCreator.prototype.first.getY(),
            temp[0]->getStart().getX(),
            temp[0]->getStart().getY()
            );
    }

    for (int i = 0;
         i < polygonCreator.prototype.sides_.size();
         i++)
    {
        painter.drawEllipse(
            QPointF(temp[i]->getEnd().getX(),
                    temp[i]->getEnd().getY()),
            Style::PointRadius,
            Style::PointRadius
            );

        temp[i]->Draw(painter, true);
    }

    // Путь луча по нажатиям "Следующее отражение"
    const auto &hist = polygonCreator.history_;
    if (hist.size() > 1)
    {
        QPen tracePen(QColor(255, 204, 0));   // жёлтый
        tracePen.setWidth(Style::TraceWidth);
        painter.setPen(tracePen);

        for (size_t k = 1; k < hist.size(); ++k)
        {
            const QPointF from(hist[k - 1].getX(), hist[k - 1].getY());
            QPointF to(hist[k].getX(), hist[k].getY());

            // Последний отрезок во время анимации растёт от нуля до полной длины
            if (animating_ && k == hist.size() - 1)
                to = from + (to - from) * animProgress_;

            painter.drawLine(from, to);
        }
    }

    // Оранжевая точка подсвечивает последнее достигнутое отражение.
    // Когда луч долетает до следующего, прежняя точка исчезает
    // (во время анимации ещё горит та, от которой луч стартовал).
    if (hist.size() > 1)
    {
        QPen reflectPen(QColor(255, 140, 0));
        reflectPen.setWidth(Style::ReflectPointWidth);
        reflectPen.setCapStyle(Qt::RoundCap);
        painter.setPen(reflectPen);

        const size_t reached = animating_ ? hist.size() - 2 : hist.size() - 1;

        // индекс 0 - источник луча, он красный и рисуется отдельно
        if (reached >= 1)
            painter.drawPoint(QPointF(hist[reached].getX(), hist[reached].getY()));
    }

    if (polygonCreator.prototype.isBeamSet_)
    {
        const Beam &b = polygonCreator.prototype.beam_;
        QPen beamPen(Qt::red);
        beamPen.setWidth(Style::WallWidth);
        painter.setPen(beamPen);
        painter.setBrush(Qt::red);
        painter.drawEllipse(QPointF(b.getX(), b.getY()),
                            Style::BeamRadius,
                            Style::BeamRadius);
    }
}
