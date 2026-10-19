#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include "frontendoperators.h"
#include <QPainter>
#include <QVector>
#include <QPair>
#include <vector>
#include <utility>

class QVBoxLayout;
class QVariantAnimation;

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

    // Анимация отражения
    bool IgnoreDuringAnimation();   // true + сообщение, если идёт анимация
    void FinishReflection();        // конец анимации отражения
    void FlushPendingNotes();       // показать сообщения, отложенные до конца анимации

private:
    Ui::MainWindow *ui;
    FrontEndOperators polygonCreator;
    QVBoxLayout *chatLayout = nullptr;

    QVariantAnimation *reflectAnim_ = nullptr;
    bool   animating_ = false;                              // идёт анимация отражения
    double animProgress_ = 1.0;                             // 0..1, доля пройденного отрезка
    int    reflectionAnimMs_ = Style::ReflectionAnimMs;     // длительность анимации
    bool   bufferNotes_ = false;                            // откладывать сообщения до конца анимации
    std::vector<std::pair<MsgType, QString>> pendingNotes_;
};
#endif // MAINWINDOW_H
