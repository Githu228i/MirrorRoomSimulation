#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "changeorconnectdialog.h"
#include "changepointdialog.h"

#include <QPen>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(1600,1200);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    int x = event->position().x();
    int y = event->position().y();

    if (y > 1000)
        return;

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
}
