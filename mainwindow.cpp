#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "changeorconnectdialog.h"
#include "changepointdialog.h"
#include "beamangledialog.h"
#include "savehistorydialog.h"
#include "Darktheme.h"

#include <QPen>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QTime>
#include <QVariantAnimation>
#include <QSlider>
#include <QEvent>
#include <QTimer>
#include <QCheckBox>
#include <QTextEdit>
#include <QKeyEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>
#include <QStandardPaths>
#include <QDateTime>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setStyleSheet(Theme::MainStyle());
    setFixedSize(Style::WindowWidth, Style::WindowHeight);

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

    // Ползунок скорости луча (виджеты horizontalSlider и label созданы в .ui)
    ui->horizontalSlider->setRange(Style::RaySpeedMin, Style::RaySpeedMax);
    ui->horizontalSlider->setSingleStep(Style::RaySpeedStep);
    ui->horizontalSlider->setPageStep(Style::RaySpeedStep * 2);
    ui->horizontalSlider->setValue(static_cast<int>(raySpeed_));
    SetRaySpeed(ui->horizontalSlider->value());   // подпись и начальное значение
    connect(ui->horizontalSlider, &QSlider::valueChanged, this,
            [this](int v) { SetRaySpeed(v); });
    ui->horizontalSlider->installEventFilter(this);   // блокировка с сообщением в чат

    // Ползунок задержки между отражениями в авторежиме (в миллисекундах)
    ui->horizontalSlider_2->setRange(Style::RepeatDelayMin, Style::RepeatDelayMax);
    ui->horizontalSlider_2->setSingleStep(Style::RepeatDelayStep);
    ui->horizontalSlider_2->setPageStep(Style::RepeatDelayStep * 2);
    ui->horizontalSlider_2->setValue(repeatDelayMs_);
    SetRepeatDelay(ui->horizontalSlider_2->value());   // подпись и начальное значение
    connect(ui->horizontalSlider_2, &QSlider::valueChanged, this,
            [this](int v) { SetRepeatDelay(v); });
    ui->horizontalSlider_2->installEventFilter(this);

    // Таймер паузы между отражениями в повторе
    delayTimer_ = new QTimer(this);
    delayTimer_->setSingleShot(true);
    connect(delayTimer_, &QTimer::timeout, this, &MainWindow::OnDelayFinished);

    // Удаление старых отражений: галочка и максимальное количество (поле ввода)
    ui->checkBox->setChecked(eraseOld_);
    ui->textEdit->setPlainText(QString::number(traceLimit_));
    connect(ui->checkBox, &QCheckBox::toggled, this, [this](bool on)
            {
                eraseOld_ = on;
                polygonCreator.ResetTrace();   // новый режим - путь луча сбрасывается
                UpdateButtons();
                update();
                AddMessage(on
                               ? QString("Удаление старых отражений включено: на экране остаются "
                                         "последние %1. Путь луча сброшен.").arg(traceLimit_)
                               : QString("Удаление старых отражений выключено. Путь луча сброшен."));
            });
    ui->checkBox->installEventFilter(this);
    ui->textEdit->installEventFilter(this);              // Enter / потеря фокуса, блокировка
    ui->textEdit->viewport()->installEventFilter(this);  // блокировка кликов мышью

    // Нажатие любой кнопки сначала применяет введённое, но ещё не подтверждённое число
    for (QPushButton *b : {ui->pushButton, ui->pushButton_2, ui->pushButton_3, ui->pushButton_4,
                           ui->pushButton_5})
        connect(b, &QPushButton::pressed, this, [this]() { ui->textEdit->clearFocus(); });

    // Настройки симуляции видны только в режиме симуляции
    ShowSimControls(false);

    // Сообщения от FrontEndOperators (ошибки ввода, отражения и т.д.) идут в чат
    polygonCreator.notify = [this](NoteLevel level, const QString &text) {
        MsgType t = MsgType::Info;
        if (level == NoteLevel::Warning)
            t = MsgType::Warning;
        else if (level == NoteLevel::Error)
            t = MsgType::Error;
        else if (level == NoteLevel::Reflection)
            t = MsgType::Reflection;
        if (bufferNotes_)
            pendingNotes_.push_back({t, text});   // покажем в конце анимации
        else
            AddMessage(text, t);
    };

    AddMessage(
        QString("Режим редактирования комнаты. Кликайте, чтобы ставить точки "
                "(комната - от %1 до %2 сторон). Клик по вершине - изменить или соединить её. "
                "«Начать симуляцию» замкнёт комнату.")
            .arg(Limits::MinWalls)
            .arg(Limits::MaxWalls));

    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::FirstButClicked);

    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::SecondButClicked);

    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::ThirdButClicked);

    ui->pushButton_3->setText("Следующее отражение");

    // Кнопка 4: повтор анимации (запуск / остановка)
    connect(ui->pushButton_4, &QPushButton::clicked,
            this, &MainWindow::FourthButClicked);

    // Кнопка 5: сохранить историю отражений в файл (доступна в обоих режимах)
    connect(ui->pushButton_5, &QPushButton::clicked,
            this, &MainWindow::FifthButClicked);

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
//  - во время анимации и повтора кнопки приглушены, но остаются нажимаемыми:
//    нажатие игнорируется с сообщением в чат;
//  - во время повтора кнопка 4 работает как "Остановить повтор", а после
//    запроса остановки приглушается тоже, пока не закончится анимация.
void MainWindow::UpdateButtons()
{
    const bool picking = !Redacting && Beaming;
    const bool looped = polygonCreator.looped_;
    const bool busy = animating_ || repeating_;

    // enabled - принимает ли кнопка нажатия, dim - приглушена ли она
    auto setState = [](QPushButton *b, bool enabled, bool dim)
    {
        b->setEnabled(enabled);

        if (dim)
        {
            auto *effect = new QGraphicsOpacityEffect(b);
            effect->setOpacity(Style::DimOpacity);
            b->setGraphicsEffect(effect);   // старый эффект Qt удаляет сам
        }
        else
        {
            b->setGraphicsEffect(nullptr);
        }
    };

    setState(ui->pushButton,   true,                busy);
    setState(ui->pushButton_2, !picking,            picking || busy);
    setState(ui->pushButton_3, !picking && !looped, picking || looped || busy);

    if (repeating_)
    {
        setState(ui->pushButton_4, true, stopRequested_);
        ui->pushButton_4->setText(stopRequested_ ? "Останавливаю..." : "Остановить повтор");
    }
    else
    {
        setState(ui->pushButton_4, !picking && !looped, picking || looped || animating_);
        ui->pushButton_4->setText("Повтор анимации");
    }

    // Кнопка 5 (сохранение истории): недоступна, пока истории нет или выбирается источник;
    // во время анимации/повтора приглушена, нажатие игнорируется с сообщением в чат.
    // Текст: "текущая" история, пока путь на экране, и "прошлой симуляции" после сброса.
    const bool hasHistory = polygonCreator.history_.size() > 1;
    setState(ui->pushButton_5, hasHistory && !picking, !hasHistory || picking || busy);
    ui->pushButton_5->setText(polygonCreator.HasTrace()
                                  ? "Сохранить текущую историю отражений"
                                  : "Сохранить историю прошлой симуляции");

    // Ползунок скорости и его подпись: тускнеют, когда менять скорость нельзя.
    // Сам ползунок остаётся enabled, чтобы eventFilter мог показать сообщение в чат.
    auto dimWidget = [](QWidget *w, bool on)
    {
        if (on)
        {
            auto *effect = new QGraphicsOpacityEffect(w);
            effect->setOpacity(Style::DimOpacity);
            w->setGraphicsEffect(effect);   // старый эффект Qt удаляет сам
        }
        else
        {
            w->setGraphicsEffect(nullptr);
        }
    };
    dimWidget(ui->horizontalSlider, SettingsBlocked());
    dimWidget(ui->label, SettingsBlocked());
    dimWidget(ui->horizontalSlider_2, SettingsBlocked());
    dimWidget(ui->label_2, SettingsBlocked());
    dimWidget(ui->checkBox, SettingsBlocked());
    dimWidget(ui->label_3, TraceLimitBlocked());
    dimWidget(ui->textEdit, TraceLimitBlocked());
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
    case MsgType::Reflection:
        bg = "#24512f";
        tag = "Отражение";
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

    // Окошко не разрастается бесконечно: самые старые сообщения (внизу) удаляем
    while (chatLayout->count() - 1 > Style::MaxChatMessages)
    {
        QLayoutItem *old = chatLayout->takeAt(chatLayout->count() - 2);
        delete old->widget();
        delete old;
    }
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
        ShowSimControls(false);
        const bool hadPath = polygonCreator.HasTrace();
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

        // Сторон в замкнутой комнате: у незамкнутой добавится замыкающая
        const int finalWalls = static_cast<int>(poly.sides_.size()) + (poly.isCycled_ ? 0 : 1);
        if (finalWalls < Limits::MinWalls) {
            AddMessage(
                QString("Нельзя начать симуляцию: слишком мало сторон. В комнате должно быть "
                        "от %1 до %2 сторон, а сейчас получится %3. Поставьте ещё точки.")
                    .arg(Limits::MinWalls)
                    .arg(Limits::MaxWalls)
                    .arg(finalWalls),
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
        ShowSimControls(true);

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
                kind = QString("вогнутая, R = %1").arg(m.getRadius(), 0, 'f', Style::MessageDecimals);
            else if (dynamic_cast<const ConvexMirror *>(&m))
                kind = QString("выпуклая, R = %1").arg(m.getRadius(), 0, 'f', Style::MessageDecimals);

            desc += QString("\nТочка %1: (%2; %3), стена %1 - %4")
                        .arg(static_cast<int>(i) + 1)
                        .arg(m.getStart().getX(), 0, 'f', Style::MessageDecimals)
                        .arg(m.getStart().getY(), 0, 'f', Style::MessageDecimals)
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
                           .arg(poly.beam_.getX(), 0, 'f', Style::MessageDecimals)
                           .arg(poly.beam_.getY(), 0, 'f', Style::MessageDecimals)
                           .arg(poly.beamWall_ + 1)
                           .arg(poly.beamWallAngle_, 0, 'f', BeamAngle::Decimals));
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

    StartReflectionStep();
}

// Кнопка 4: повтор анимации. Первое нажатие запускает отражения одно за другим,
// второе просит остановиться после текущей анимации, а до её конца кнопка
// (как и все остальные) нажатия игнорирует.
void MainWindow::FourthButClicked()
{
    if (repeating_)
    {
        // Идёт пауза между отражениями: останавливаем сразу, паузу сбрасываем,
        // следующее отражение не делаем, все кнопки сразу снова активны
        if (delaying_)
        {
            delayTimer_->stop();
            delaying_ = false;
            repeating_ = false;
            stopRequested_ = false;
            AddMessage("Повтор остановлен: пауза между отражениями прервана.");
            UpdateButtons();
            update();
            return;
        }

        if (stopRequested_)
        {
            AddMessage(
                "Остановка повтора уже запрошена: дождитесь конца анимации.",
                MsgType::Warning);
            return;
        }

        stopRequested_ = true;
        AddMessage("Повтор остановится после завершения текущей анимации.");
        UpdateButtons();
        return;
    }

    // Одиночная анимация (кнопка 3) ещё идёт
    if (IgnoreDuringAnimation())
        return;

    if (Redacting || Beaming || !polygonCreator.prototype.isBeamSet_
        || polygonCreator.looped_)
        return;

    repeating_ = true;
    stopRequested_ = false;

    if (!StartReflectionStep())
    {
        // первый шаг не получился (причина уже в чате)
        repeating_ = false;
        UpdateButtons();
        return;
    }

    AddMessage(
        "Повтор запущен: луч отражается сам. Нажмите кнопку ещё раз, "
        "чтобы остановить (или повтор закончится, когда луч зациклится).");
}

// Кнопка 5: сохраняет всю историю отражений (history_) в txt-файл, выбранный пользователем.
// История хранится и после сброса пути, поэтому можно сохранить и прошлую симуляцию.
// В файл попадают комната и параметры луча на момент старта той симуляции (snapshot_).
void MainWindow::FifthButClicked()
{
    if (animating_ || repeating_)
    {
        AddMessage(
            "Нельзя сохранить историю, пока идёт анимация или повтор. "
            "Дождитесь конца анимации или остановите повтор.",
            MsgType::Warning);
        return;
    }

    if (!Redacting && Beaming)
    {
        AddMessage(
            "Нельзя сохранить историю, пока не выбран источник на стене.",
            MsgType::Warning);
        return;
    }

    // копии на время диалогов
    const std::vector<Points> hist = polygonCreator.history_;
    const std::vector<Beam> states = polygonCreator.states_;
    const TraceSnapshot snap = polygonCreator.snapshot_;

    if (hist.size() <= 1)
    {
        AddMessage("Сохранять нечего: история отражений пуста.", MsgType::Warning);
        return;
    }

    const bool current = polygonCreator.HasTrace();

    // 1) Сколько знаков после запятой сохранять в координатах
    SaveHistoryDialog precisionDialog(
        saveDecimals_, Style::SaveDecimalsMax,
        hist[1].getX(), hist[1].getY(), this);
    precisionDialog.move(
        this->geometry().center()
        - precisionDialog.rect().center()
        );

    if (precisionDialog.exec() != QDialog::Accepted)
    {
        AddMessage("Сохранение истории отменено.");
        return;
    }
    saveDecimals_ = precisionDialog.getDecimals();

    // 2) Куда сохранять
    if (lastSaveDir_.isEmpty())
        lastSaveDir_ = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    const QString defaultName =
        lastSaveDir_ + "/reflections_"
        + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";

    QString path = QFileDialog::getSaveFileName(
        this, "Сохранить историю отражений", defaultName, "Текстовые файлы (*.txt)");

    if (path.isEmpty())
    {
        AddMessage("Сохранение истории отменено.");
        return;
    }

    // Файл всегда .txt; если пришлось дописать расширение, перезапись не подтверждалась
    // диалогом, поэтому существующий файл не трогаем
    if (QFileInfo(path).suffix().compare("txt", Qt::CaseInsensitive) != 0)
    {
        path += ".txt";
        if (QFileInfo::exists(path))
        {
            AddMessage(
                QString("Файл «%1» уже существует. Выберите другое имя или укажите "
                        "расширение .txt, чтобы перезаписать файл.").arg(path),
                MsgType::Error);
            return;
        }
    }

    lastSaveDir_ = QFileInfo(path).absolutePath();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        AddMessage(
            QString("Не удалось открыть файл для записи: %1 (%2).")
                .arg(path, file.errorString()),
            MsgType::Error);
        return;
    }

    // Форматирование чисел
    const int dec = saveDecimals_;
    auto num = [dec](double v) { return QString::number(v, 'f', dec); };
    auto pt = [&num](double x, double y)
    {
        return QString("(%1; %2)").arg(num(x), num(y));
    };
    // Угол луча (радианы) -> градусы в пределах [0; 360)
    auto ang = [](double rad)
    {
        double d = std::fmod(rad * Math::HalfTurnDeg / Math::Pi, Math::FullTurnDeg);
        if (d < 0.0)
            d += Math::FullTurnDeg;
        return QString::number(d, 'f', Style::SaveAngleDecimals);
    };

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "История отражений луча\n";
    out << "Сохранено: "
        << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
    out << (current ? "Симуляция: текущая\n" : "Симуляция: прошлая (путь был сброшен)\n");
    out << "Знаков после запятой в координатах: " << dec << "\n";
    out << "Углы направления луча: в градусах, из внутреннего представления луча, "
           "в пределах от 0 до 360\n\n";

    // Комната
    out << "КОМНАТА\n";
    out << "Сторон: " << snap.walls.size() << "\n";
    for (size_t i = 0; i < snap.walls.size(); ++i)
    {
        const auto &w = snap.walls[i];

        QString kind = "плоская";
        if (w.type == MirrorKind::Concave)
            kind = "вогнутая, R = " + num(w.radius);
        else if (w.type == MirrorKind::Convex)
            kind = "выпуклая, R = " + num(w.radius);

        out << "Точка " << (i + 1) << ": " << pt(w.x, w.y)
            << ", стена " << (i + 1) << " - " << kind << "\n";
    }

    // Начальные параметры луча
    out << "\nНАЧАЛЬНЫЙ ЛУЧ\n";
    out << "Источник: " << pt(hist[0].getX(), hist[0].getY())
        << " на стене " << (snap.beamWall + 1) << "\n";
    out << "Угол к стене: "
        << QString::number(snap.beamWallAngle, 'f', Style::SaveAngleDecimals) << "°\n";
    if (!states.empty())
        out << "Направление луча: " << ang(states[0].getA()) << "°\n";

    // Отражения: после каждой точки - направление луча, уходящего от неё
    out << "\nОТРАЖЕНИЯ\n";
    out << "Количество отражений: " << (hist.size() - 1) << "\n";
    for (size_t k = 1; k < hist.size(); ++k)
    {
        out << "Отражение " << k << ": " << pt(hist[k].getX(), hist[k].getY());
        if (k < states.size())
            out << ", угол луча после отражения: " << ang(states[k].getA()) << "°";
        out << "\n";
    }

    out.flush();

    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError)
    {
        AddMessage(
            QString("Ошибка при записи файла: %1 (%2).").arg(path, file.errorString()),
            MsgType::Error);
        file.close();
        return;
    }
    file.close();

    AddMessage(
        QString("История отражений (%1) сохранена в файл: %2")
            .arg(hist.size() - 1)
            .arg(path));
}

// Считает следующее отражение и запускает анимацию отрезка до него.
// Длительность - длина отрезка, делённая на скорость луча.
// false - шаг не получился (сообщение об ошибке уже в чате).
bool MainWindow::StartReflectionStep()
{
    // Сообщения о шаге (точка отражения, зацикливание) покажем в конце анимации
    pendingNotes_.clear();
    bufferNotes_ = true;

    if (!polygonCreator.NextReflection())
    {
        FlushPendingNotes();
        return false;
    }

    const auto &hist = polygonCreator.history_;
    const double length = hist[hist.size() - 2].Dist(hist.back());

    int ms = static_cast<int>(std::lround(length / raySpeed_ * Style::MsPerSecond));
    if (ms < Style::MinAnimMs)
        ms = Style::MinAnimMs;

    animating_ = true;
    animProgress_ = 0.0;
    reflectAnim_->setDuration(ms);
    reflectAnim_->start();
    UpdateButtons();   // на время анимации кнопки тускнеют
    update();
    return true;
}

// Сохраняет скорость луча и обновляет подпись. Новая скорость
// применяется к следующему отрезку (во время анимации ползунок заблокирован).
void MainWindow::SetRaySpeed(double speed)
{
    raySpeed_ = speed;
    ui->label->setText(QString("Скорость луча: %1 пикс/с").arg(speed, 0, 'f', 0));
}

// Сохраняет задержку между отражениями (мс) и обновляет подпись
void MainWindow::SetRepeatDelay(int ms)
{
    repeatDelayMs_ = ms;
    ui->label_2->setText(
        QString("Задержка луча между отражениями (в режиме авто): %1 сек")
            .arg(ms / Style::MsPerSecond, 0, 'f', Style::DelayDecimals));
}

// Пауза закончилась: следующее отражение (если повтор не остановили)
void MainWindow::OnDelayFinished()
{
    delaying_ = false;

    if (!repeating_)
        return;

    if (StartReflectionStep())
        return;   // анимация следующего отрезка уже идёт

    // шаг не получился (причина уже в чате)
    repeating_ = false;
    stopRequested_ = false;
    AddMessage("Повтор остановлен.");
    UpdateButtons();
    update();
}

// Оба ползунка и их подписи видны только в режиме симуляции
void MainWindow::ShowSimControls(bool on)
{
    ui->horizontalSlider->setVisible(on);
    ui->label->setVisible(on);
    ui->horizontalSlider_2->setVisible(on);
    ui->label_2->setVisible(on);
    ui->checkBox->setVisible(on);
    ui->label_3->setVisible(on);
    ui->textEdit->setVisible(on);
}

// Скорость и задержку нельзя менять во время анимации/повтора
// и пока выбирается источник луча
bool MainWindow::SettingsBlocked() const
{
    return animating_ || repeating_ || (!Redacting && Beaming);
}

// Поле «Макс. кол-во отражений» работает только при включённой галочке
// «Удалять старые отражения» (и когда остальные настройки не заблокированы)
bool MainWindow::TraceLimitBlocked() const
{
    return SettingsBlocked() || !eraseOld_;
}

// Применяет число из поля «Макс. кол-во отражений» (Enter или потеря фокуса).
// То же самое число - ничего не сбрасывается; новое - путь луча сбрасывается.
void MainWindow::CommitTraceLimit()
{
    const QString current = QString::number(traceLimit_);

    // Поле скрыто или заблокировано: введённое не применяем, возвращаем прежнее
    if (!ui->textEdit->isVisible() || TraceLimitBlocked())
    {
        if (ui->textEdit->toPlainText() != current)
            ui->textEdit->setPlainText(current);
        return;
    }

    const QString typed = ui->textEdit->toPlainText().trimmed();
    bool ok = false;
    const int value = typed.toInt(&ok);

    if (!ok || value < Style::TraceLimitMin || value > Style::TraceLimitMax)
    {
        AddMessage(
            QString("Максимальное количество отражений: нужно целое число от %1 до %2 "
                    "(введено «%3»). Оставлено прежнее значение: %4.")
                .arg(Style::TraceLimitMin)
                .arg(Style::TraceLimitMax)
                .arg(typed, current),
            MsgType::Error);
        ui->textEdit->setPlainText(current);
        return;
    }

    ui->textEdit->setPlainText(QString::number(value));   // например, «050» -> «50»

    if (value == traceLimit_)
        return;   // число то же самое - ничего не сбрасываем

    traceLimit_ = value;
    polygonCreator.ResetTrace();
    UpdateButtons();
    update();

    AddMessage(QString("Максимальное количество отражений: %1. Путь луча сброшен.")
                   .arg(traceLimit_));
}

// Enter и потеря фокуса применяют число в поле ввода. Пока настройки заблокированы
// (а для поля числа - ещё и пока выключена галочка), глотаем нажатия, колесо и клавиши
// и пишем в чат.
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    const bool isLimitField = (obj == ui->textEdit || obj == ui->textEdit->viewport());

    if (obj == ui->textEdit)
    {
        if (event->type() == QEvent::FocusOut)
        {
            CommitTraceLimit();
            return false;   // фокус уходит как обычно
        }

        if (!TraceLimitBlocked() && event->type() == QEvent::KeyPress)
        {
            const int key = static_cast<QKeyEvent *>(event)->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter)
            {
                ui->textEdit->clearFocus();   // потеря фокуса применит значение
                return true;                  // перевода строки в поле не будет
            }
        }
    }

    QString what;
    if (obj == ui->horizontalSlider)
        what = "скорость луча";
    else if (obj == ui->horizontalSlider_2)
        what = "задержку луча";
    else if (obj == ui->checkBox)
        what = "режим удаления старых отражений";
    else if (isLimitField)
        what = "максимальное количество отражений";

    const bool blocked = isLimitField ? TraceLimitBlocked() : SettingsBlocked();

    if (!what.isEmpty() && blocked)
    {
        switch (event->type())
        {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
        case QEvent::KeyPress:
            if (animating_ || repeating_)
                AddMessage(
                    QString("Нельзя менять %1, пока идёт анимация или повтор. "
                            "Дождитесь конца анимации или остановите повтор.").arg(what),
                    MsgType::Warning);
            else if (!Redacting && Beaming)
                AddMessage(
                    QString("Нельзя менять %1, пока не выбран источник на стене. "
                            "Кликните по стене, откуда должен выйти луч.").arg(what),
                    MsgType::Warning);
            else
                AddMessage(
                    QString("Нельзя менять %1, пока не включена галочка "
                            "«Удалять старые отражения».").arg(what),
                    MsgType::Warning);
            return true;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Если идёт анимация или повтор - сообщает об этом в чат,
// и вызывающий код должен выйти
bool MainWindow::IgnoreDuringAnimation()
{
    if (!animating_ && !repeating_)
        return false;

    AddMessage(
        "Нельзя редактировать, пока идёт симуляция: "
        "дождитесь конца анимации отражения или остановите повтор.",
        MsgType::Warning);
    return true;
}

void MainWindow::FinishReflection()
{
    animating_ = false;
    animProgress_ = 1.0;

    FlushPendingNotes();

    // Повтор: сразу следующее отражение, пока не просили остановиться
    // и луч не зациклился
    if (repeating_)
    {
        if (!stopRequested_ && !polygonCreator.looped_)
        {
            // Пауза между отражениями: кнопка 4 остаётся активной и её можно
            // нажать, чтобы остановить повтор прямо во время паузы
            if (repeatDelayMs_ > 0)
            {
                delaying_ = true;
                delayTimer_->start(repeatDelayMs_);
                UpdateButtons();
                update();
                return;
            }

            if (StartReflectionStep())
                return;   // анимация следующего отрезка уже идёт
        }

        const bool looped = polygonCreator.looped_;
        repeating_ = false;
        stopRequested_ = false;
        AddMessage(looped ? "Повтор остановлен: луч зациклился."
                          : "Повтор остановлен.");
    }

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
    // Клик по окну убирает фокус с поля ввода (введённое число применится)
    if (ui->textEdit->hasFocus())
        ui->textEdit->clearFocus();

    if (IgnoreDuringAnimation())
        return;

    int x = event->position().x();
    int y = event->position().y();

    if (x > Limits::MaxX || y > Limits::MaxY)
    {
        AddMessage(
            QString("Клик вне рабочей области (допустимо: x от %1 до %2, y от %3 до %4).")
                .arg(Limits::MinX, 0, 'f', 0)
                .arg(Limits::MaxX, 0, 'f', 0)
                .arg(Limits::MinY, 0, 'f', 0)
                .arg(Limits::MaxY, 0, 'f', 0),
            MsgType::Warning);
        return;
    }
    // Клик по точке источника - изменить угол (в любом режиме)
    if (polygonCreator.prototype.isBeamSet_)
    {
        const Beam &b = polygonCreator.prototype.beam_;
        double bx = x - b.getX();
        double by = y - b.getY();

        if (bx * bx + by * by <= Limits::ClickRadius * Limits::ClickRadius)
        {
            BeamAngleDialog angleDialog(
                this,
                polygonCreator.prototype.beamWallAngle_
                );
            connect(&angleDialog, &BeamAngleDialog::invalidAngle, this,
                    [this](const QString &msg) { AddMessage(msg, MsgType::Error); });
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
                               .arg(angleDialog.getAngle(), 0, 'f', BeamAngle::Decimals)
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
            polygonCreator.prototype.PlaceBeam(Points(x, y), Limits::BeamWallTolerance, Limits::MinBeamVertexDist);

        if (res == BeamPlaceResult::NoWall)
        {
            AddMessage(
                QString("Невозможно поставить луч: рядом нет стены. "
                        "Кликните точно по линии зеркала (не дальше %1 пикселей).")
                    .arg(static_cast<int>(Limits::BeamWallTolerance)),
                MsgType::Error);
            return;
        }

        if (res == BeamPlaceResult::TooClose)
        {
            AddMessage(
                QString("Луч нельзя ставить так близко к вершине "
                        "(ближе %1 пикселей). Выберите точку подальше.")
                    .arg(static_cast<int>(Limits::MinBeamVertexDist)),
                MsgType::Error);
            return;
        }

        repaint();   // показать точку, пока открыто окно выбора угла

        BeamAngleDialog angleDialog(this);
        connect(&angleDialog, &BeamAngleDialog::invalidAngle, this,
                [this](const QString &msg) { AddMessage(msg, MsgType::Error); });
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
                           .arg(poly.beam_.getX(), 0, 'f', Style::MessageDecimals)
                           .arg(poly.beam_.getY(), 0, 'f', Style::MessageDecimals)
                           .arg(poly.beamWall_ + 1)
                           .arg(angleDialog.getAngle(), 0, 'f', BeamAngle::Decimals));
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
        if (dx * dx + dy * dy <= Limits::ClickRadius * Limits::ClickRadius)
        {
            // -------------------------------------------------
            // ПОЛИГОН УЖЕ ЗАМКНУТ
            // Диалог выбора, но «Подключить» недоступно
            // -------------------------------------------------

            if (polygonCreator.prototype.isCycled_)
            {
                ChangeOrConnectDialog dialog(this, true, false);

                dialog.move(
                    this->geometry().center()
                    - dialog.rect().center()
                    );

                const int choice = dialog.exec();

                if (choice == QDialog::Rejected)
                {
                    AddMessage("Действие с точкой отменено.");
                    return;
                }

                if (choice == ChangeOrConnectDialog::DeleteCode)
                {
                    polygonCreator.DeletePoint(i);   // удалит вершину, если останется не меньше Limits::MinWalls
                    update();
                    return;
                }

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

            // Нажали "Удалить"
            if (choice == ChangeOrConnectDialog::DeleteCode)
            {
                polygonCreator.DeletePoint(i);
                update();
                return;
            }

            // Accepted - "Изменить", иначе (ConnectCode) - "Подключить"
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
        if (dx * dx + dy * dy <= Limits::MinPointDist * Limits::MinPointDist)
        {
            AddMessage(QString("Нельзя поставить точку: она слишком близко к другой точке (ближе %1 пикселей).")
                           .arg(static_cast<int>(Limits::MinPointDist)),
                       MsgType::Error);

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
        if (dx * dx + dy * dy <= Limits::ClickRadius * Limits::ClickRadius)
        {
            // Последнюю точку нельзя подключить: в диалоге доступны
            // только "Изменить" и "Удалить"
            const int lastIdx = static_cast<int>(temp.size());

            ChangeOrConnectDialog dialog(this, true, false);

            dialog.move(
                this->geometry().center()
                - dialog.rect().center()
                );

            const int choice = dialog.exec();

            if (choice == QDialog::Rejected)
            {
                AddMessage("Действие с точкой отменено.");
                return;
            }

            if (choice == ChangeOrConnectDialog::DeleteCode)
            {
                polygonCreator.DeletePoint(lastIdx);
                update();
                return;
            }

            // Нажали "Изменить"

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
        if (dx * dx + dy * dy <= Limits::MinPointDist * Limits::MinPointDist)
        {
            AddMessage(QString("Нельзя поставить точку: она слишком близко к другой точке (ближе %1 пикселей).")
                           .arg(static_cast<int>(Limits::MinPointDist)),
                       MsgType::Error);

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

        // Клик по единственной точке: доступно только удаление
        if (dx * dx + dy * dy <= Limits::ClickRadius * Limits::ClickRadius)
        {
            ChangeOrConnectDialog dialog(this, false, false);

            dialog.move(
                this->geometry().center()
                - dialog.rect().center()
                );

            if (dialog.exec() == ChangeOrConnectDialog::DeleteCode)
            {
                polygonCreator.DeletePoint(0);
                update();
            }
            else
            {
                AddMessage("Действие с точкой отменено.");
            }

            return;
        }

        if (dx * dx + dy * dy <= Limits::MinPointDist * Limits::MinPointDist)
        {
            AddMessage(QString("Нельзя поставить точку: она слишком близко к другой точке (ближе %1 пикселей).")
                           .arg(static_cast<int>(Limits::MinPointDist)),
                       MsgType::Error);

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
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Тёмный фон окна и рабочей зоны (центральный виджет прозрачный, см. Darktheme.h)
    painter.fillRect(rect(), Theme::Window);
    const QRectF zone(Limits::MinX, Limits::MinY,
                      Limits::MaxX - Limits::MinX, Limits::MaxY - Limits::MinY);
    painter.fillRect(zone, Theme::Canvas);
    painter.setPen(QPen(Theme::CanvasBorder, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(zone);

    QPen pen(Theme::Wall);
    pen.setWidth(Style::WallWidth);

    painter.setPen(pen);
    painter.setBrush(Theme::Wall);

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
    if (polygonCreator.HasTrace())
    {
        QPen tracePen(Theme::Trace);   // жёлтый
        tracePen.setWidth(Style::TraceWidth);
        painter.setPen(tracePen);

        // Если включено удаление, остаются только последние traceLimit_ отрезков (вместе с растущим).
        // Старый отрезок пропадает сразу, без анимации, как только начинается новый.
        // Сама история history_ не обрезается: по ней ищется зацикливание.
        size_t firstSeg = 1;
        const size_t maxLines = eraseOld_ ? static_cast<size_t>(traceLimit_) : size_t(0);
        if (maxLines > 0 && hist.size() - 1 > maxLines)
            firstSeg = hist.size() - maxLines;

        for (size_t k = firstSeg; k < hist.size(); ++k)
        {
            QPointF from(hist[k - 1].getX(), hist[k - 1].getY());
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
    if (polygonCreator.HasTrace())
    {
        QPen reflectPen(Theme::Reflect);
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
        QPen beamPen(Theme::Source);
        beamPen.setWidth(Style::WallWidth);
        painter.setPen(beamPen);
        painter.setBrush(Theme::Source);
        painter.drawEllipse(QPointF(b.getX(), b.getY()),
                            Style::BeamRadius,
                            Style::BeamRadius);
    }
}
