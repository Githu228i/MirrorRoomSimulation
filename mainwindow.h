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
class QTimer;

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
    void LoadRoomFromFile();        // кнопка 3 в режиме редактирования: комната и луч из txt-файла
    void UpdateButtons();

    // Служебное окно справа
    enum class MsgType { Info, Warning, Error, Reflection };
    void AddMessage(const QString &text, MsgType type = MsgType::Info);

    // Анимация отражения
    void FourthButClicked();        // повтор анимации (запуск / остановка)
    void FifthButClicked();         // сохранить историю отражений в txt-файл
    bool StartReflectionStep();     // следующее отражение + анимация отрезка до него
    bool IgnoreDuringAnimation();   // true + сообщение, если идёт анимация или повтор
    void FinishReflection();        // конец анимации отражения
    void FlushPendingNotes();       // показать сообщения, отложенные до конца анимации

    // Скорость луча (ползунок)
    void SetRaySpeed(double speed);          // сохранить скорость и обновить подпись
    bool SettingsBlocked() const;            // true, если менять настройки симуляции сейчас нельзя
    bool eventFilter(QObject *obj, QEvent *event) override;   // блокировка ползунка с сообщением в чат

    // Задержка между отражениями в авторежиме (второй ползунок)
    void SetRepeatDelay(int ms);             // сохранить задержку и обновить подпись
    void OnDelayFinished();                  // задержка закончилась - следующее отражение
    void ShowSimControls(bool on);           // показать/скрыть настройки симуляции (ползунки, галочка, поле)
    void CommitTraceLimit();                 // применить число из поля «Макс. кол-во отражений»
    bool TraceLimitBlocked() const;          // поле «Макс. кол-во отражений» недоступно (настройки заблокированы или галочка выключена)

private:
    Ui::MainWindow *ui;
    FrontEndOperators polygonCreator;
    QVBoxLayout *chatLayout = nullptr;

    QVariantAnimation *reflectAnim_ = nullptr;
    bool   animating_ = false;                              // идёт анимация отражения
    double animProgress_ = 1.0;                             // 0..1, доля пройденного отрезка
    double raySpeed_ = Style::RaySpeed;                     // скорость луча, пикселей в секунду
    bool   repeating_ = false;                              // идёт повтор анимации (кнопка 4)
    bool   stopRequested_ = false;                          // повтор остановится после текущей анимации
    bool   bufferNotes_ = false;                            // откладывать сообщения до конца анимации
    std::vector<std::pair<MsgType, QString>> pendingNotes_;
    QTimer *delayTimer_ = nullptr;                          // пауза между отражениями в повторе
    bool   delaying_ = false;                               // идёт пауза между отражениями
    int    repeatDelayMs_ = Style::RepeatDelayDefault;      // длительность паузы, мс
    bool   eraseOld_ = Style::EraseOldDefault;              // удалять старые отражения (галочка)
    int    traceLimit_ = Style::TraceLimitDefault;          // сколько последних отражений остаётся на экране
    QString lastSaveDir_;                                   // папка последнего сохранения истории
    QString lastLoadDir_;                                   // папка последней загрузки комнаты из файла
    int     saveDecimals_ = Style::SaveDecimals;            // знаков после запятой в координатах при сохранении
};
#endif // MAINWINDOW_H
