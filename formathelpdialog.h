#ifndef FORMATHELPDIALOG_H
#define FORMATHELPDIALOG_H

#include <QDialog>

class QLabel;
class QPushButton;

// Справка: как должен выглядеть txt-файл для загрузки комнаты и луча.
// Только информация, единственная кнопка - «Закрыть» (Esc тоже закрывает)
class FormatHelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FormatHelpDialog(QWidget *parent = nullptr);

private:
    QLabel *titleLabel;
    QLabel *bodyLabel;
    QPushButton *closeButton;
};

#endif // FORMATHELPDIALOG_H
