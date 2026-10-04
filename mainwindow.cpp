#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QPen>

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
    polygonCreator.CreatePoint(x, y);
    update();
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
    auto temp = polygonCreator.prototype.sides_;
    if (!polygonCreator.prototype.isExist()) return;
    painter.drawEllipse(polygonCreator.prototype.first.getX() - 4, polygonCreator.prototype.first.getY() - 4, 8, 8);
    if (!temp.empty()){
        painter.drawLine(
            polygonCreator.prototype.first.getX(),
            polygonCreator.prototype.first.getY(),
            temp[0].getStart().getX(),
            temp[0].getStart().getY()
            );
    }
    for (int i = 0; i < polygonCreator.prototype.sides_.size(); i++) {
        painter.drawEllipse(temp[i].getEnd().getX() - 4, temp[i].getEnd().getY() - 4, 8, 8);
        painter.drawLine(
            temp[i].getStart().getX(),
            temp[i].getStart().getY(),
            temp[i].getEnd().getX(),
            temp[i].getEnd().getY()
            );
    }
}
