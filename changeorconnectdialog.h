#ifndef CHANGEORCONNECTDIALOG_H
#define CHANGEORCONNECTDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;

class ChangeOrConnectDialog : public QDialog
{
    Q_OBJECT

public:
    // Результат exec(): Rejected (Esc) - отмена, Accepted - «Изменить»,
    // остальные кнопки возвращают свои коды
    static constexpr int ConnectCode = 2;   // «Подключить»
    static constexpr int DeleteCode  = 3;   // «Удалить»

    // canChange / canConnect: недоступное для этой точки действие гасится
    // (кнопка неактивна); «Удалить» доступна всегда
    explicit ChangeOrConnectDialog(QWidget *parent = nullptr,
                                   bool canChange = true,
                                   bool canConnect = true);

private:
    QLabel *textLabel;
    QPushButton *button1;
    QPushButton *button2;
    QPushButton *button3;
};

#endif // CHANGEORCONNECTDIALOG_H
