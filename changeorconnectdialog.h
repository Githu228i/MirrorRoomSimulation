#ifndef CHANGEORCONNECTDIALOG_H
#define CHANGEORCONNECTDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;

class ChangeOrConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeOrConnectDialog(QWidget *parent = nullptr);

private:
    QLabel *textLabel;
    QPushButton *button1;
    QPushButton *button2;
};

#endif // CHANGEORCONNECTDIALOG_H
