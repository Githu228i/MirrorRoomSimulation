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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(2000,1200);

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

    // Новое сообщение - прокрутка к самому низу
    QScrollBar *chatBar = ui->chatScrollArea->verticalScrollBar();
    connect(chatBar, &QScrollBar::rangeChanged, this,
            [chatBar](int, int max) { chatBar->setValue(max); });

    connect(ui->pushButton, &QPushButton::clicked,
            this, &MainWindow::FirstButClicked);

    connect(ui->pushButton_2, &QPushButton::clicked,
            this, &MainWindow::SecondButClicked);

    connect(ui->pushButton_3, &QPushButton::clicked,
            this, &MainWindow::ThirdButClicked);

    ui->pushButton_3->setText("Следующее отражение");

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

// Режим выбора точки луча: кнопки 2, 3, 4 недоступны и приглушены
void MainWindow::UpdateButtons()
{
    const bool picking = !Redacting && Beaming;

    for (QPushButton *b : {ui->pushButton_2, ui->pushButton_3, ui->pushButton_4})
    {
        b->setEnabled(!picking);

        if (picking)
        {
            auto *effect = new QGraphicsOpacityEffect(b);
            effect->setOpacity(0.35);
            b->setGraphicsEffect(effect);   // старый эффект Qt удаляет сам
        }
        else
        {
            b->setGraphicsEffect(nullptr);
        }
    }
}

// Служебное окно: добавляет сообщение-"пузырь" сверху вниз
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

    // перед растяжкой, чтобы сообщения шли сверху вниз
    chatLayout->insertWidget(chatLayout->count() - 1, bubble);
}

void MainWindow::FirstButClicked() {
    if (Redacting) {
        polygonCreator.DeleteLast();
        update();

    } else {
        Redacting = true;
        ui->pushButton->setText("Откатить точку");
        ui->pushButton_2->setText("Начать симуляцию");
        ui->pushButton_3->setVisible(false);
        ui->pushButton_4->setVisible(false);
        polygonCreator.ResetTrace();
        UpdateButtons();
        update();   // сразу перерисовать без линий отражений
    }
}

void MainWindow::SecondButClicked() {
    if (Redacting) {
        if (polygonCreator.prototype.sides_.size() > 2) {
            if (!polygonCreator.prototype.isCycled_)
                polygonCreator.Binding();
            update();
            Redacting = false;
            ui->pushButton->setText("Редактировать комнату");
            ui->pushButton_2->setText("Переставить источник света");
            ui->pushButton_3->setVisible(true);
            ui->pushButton_4->setVisible(true);
            // форма комнаты могла измениться - пересчитываем направление луча
            if (polygonCreator.prototype.isBeamSet_)
                polygonCreator.prototype.SetBeamAngle(
                    polygonCreator.prototype.beamWallAngle_);
            polygonCreator.ResetTrace();
            Beaming = !polygonCreator.prototype.isBeamSet_;
            UpdateButtons();
        }
    } else {
        polygonCreator.prototype.isBeamSet_ = false;
        polygonCreator.ResetTrace();
        Beaming = true;
        UpdateButtons();
        update();
    }
}

void MainWindow::ThirdButClicked() {
    if (Redacting || Beaming || !polygonCreator.prototype.isBeamSet_)
        return;

    polygonCreator.NextReflection();
    update();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    int x = event->position().x();
    int y = event->position().y();

    if (y > 1000)
        return;
    if (x > 1580)
        return;
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
                update();
            }
            return;
        }
    }

    if (!Redacting && !Beaming) return;
    if (!Redacting && Beaming)
    {
        BeamPlaceResult res =
            polygonCreator.prototype.PlaceBeam(Points(x, y), 6.0, 10.0);

        if (res == BeamPlaceResult::TooClose)
        {
            QMessageBox::critical(
                this,
                "ВНИМАНИЕ",
                "Точки слишком близко!"
                );
            return;
        }

        if (res == BeamPlaceResult::Placed)
        {
            repaint();

            BeamAngleDialog angleDialog(this);
            angleDialog.move(
                this->geometry().center()
                - angleDialog.rect().center()
                );

            if (angleDialog.exec() == QDialog::Accepted)
            {
                polygonCreator.prototype.SetBeamAngle(angleDialog.getAngle());
                polygonCreator.ResetTrace();
                Beaming = false;
                UpdateButtons();
            }
            else
            {
                polygonCreator.prototype.isBeamSet_ = false;
            }
            update();
        }
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

            if (dialog.exec() == QDialog::Accepted)
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
                    QMessageBox::critical(
                        this,
                        "ВНИМАНИЕ",
                        "Нельзя соединять зеркало с собой же!"
                        );

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
            QMessageBox::critical(
                this,
                "ВНИМАНИЕ",
                "Точки слишком близко!"
                );

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
            QMessageBox::critical(
                this,
                "ВНИМАНИЕ",
                "Точки слишком близко!"
                );

            return;
        }
    }

    // =========================================================
    // ЕСЛИ НЕ ПОПАЛИ НИ В ОДНУ ТОЧКУ
    // СОЗДАЁМ НОВУЮ
    // =========================================================

    if (polygonCreator.prototype.isCycled_)
        return;

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
            QMessageBox::critical(
                this,
                "ВНИМАНИЕ",
                "Точки слишком близко!"
                );

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
    pen.setWidth(5);

    painter.setPen(pen);
    painter.setBrush(Qt::black);

    const auto &temp = polygonCreator.prototype.sides_;

    if (!polygonCreator.prototype.isExist())
        return;

    painter.drawEllipse(
        polygonCreator.prototype.first.getX() - 4,
        polygonCreator.prototype.first.getY() - 4,
        8,
        8
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
            temp[i]->getEnd().getX() - 4,
            temp[i]->getEnd().getY() - 4,
            8,
            8
            );

        temp[i]->Draw(painter, true);
    }

    // Путь луча по нажатиям "Следующее отражение"
    const auto &hist = polygonCreator.history_;
    if (hist.size() > 1)
    {
        QPen tracePen(QColor(255, 204, 0));   // жёлтый
        tracePen.setWidth(2);
        painter.setPen(tracePen);

        for (size_t k = 1; k < hist.size(); ++k)
        {
            painter.drawLine(
                QPointF(hist[k - 1].getX(), hist[k - 1].getY()),
                QPointF(hist[k].getX(), hist[k].getY())
                );
        }
    }

    if (polygonCreator.prototype.isBeamSet_)
    {
        const Beam &b = polygonCreator.prototype.beam_;
        QPen beamPen(Qt::red);
        beamPen.setWidth(5);
        painter.setPen(beamPen);
        painter.setBrush(Qt::red);
        painter.drawEllipse(b.getX() - 4, b.getY() - 4, 8, 8);
    }
}
