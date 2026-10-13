#ifndef BEAMANGLEDIALOG_H
#define BEAMANGLEDIALOG_H

#include <QDialog>

class QLabel;
class QDoubleSpinBox;
class QPushButton;

class BeamAngleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BeamAngleDialog(QWidget *parent = nullptr, double initialAngle = 90.0);

    // Угол между стеной и лучом в градусах
    double getAngle() const;

private:
    QLabel *titleLabel;
    QLabel *hintLabel;
    QDoubleSpinBox *angleSpinBox;
    QPushButton *cancelButton;
    QPushButton *okButton;

    double angle_ = 90.0;

private slots:
    void applyChanges();
};

#endif // BEAMANGLEDIALOG_H
