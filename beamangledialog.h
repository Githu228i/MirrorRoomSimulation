#ifndef BEAMANGLEDIALOG_H
#define BEAMANGLEDIALOG_H

#include <QDialog>
#include <QString>

class QLabel;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QEvent;

#include "constants.h"

class BeamAngleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BeamAngleDialog(QWidget *parent = nullptr, double initialAngle = 90.0);

    // Угол между стеной и лучом в градусах
    double getAngle() const;

signals:
    // Введён недопустимый угол: диалог остаётся открытым, текст ошибки уходит в чат
    void invalidAngle(const QString &message);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QLabel *titleLabel;
    QLabel *hintLabel;
    QDoubleSpinBox *angleSpinBox;
    QLineEdit *angleEdit = nullptr;   // поле ввода внутри спинбокса
    QPushButton *cancelButton;
    QPushButton *okButton;

    double angle_ = 90.0;

private slots:
    void applyChanges();
};

#endif // BEAMANGLEDIALOG_H
