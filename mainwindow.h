#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include "frontendoperators.h"
#include <QPainter>
#include <QVector>
#include <QPair>

class QVBoxLayout;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool Redacting = true;
    bool Beaming = false;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void FirstButClicked();
    void SecondButClicked();
    void ThirdButClicked();
    void UpdateButtons();

    // Служебное окно справа
    enum class MsgType { Info, Warning, Error };
    void AddMessage(const QString &text, MsgType type = MsgType::Info);

private:
    Ui::MainWindow *ui;
    FrontEndOperators polygonCreator;
    QVBoxLayout *chatLayout = nullptr;
};
#endif // MAINWINDOW_H
