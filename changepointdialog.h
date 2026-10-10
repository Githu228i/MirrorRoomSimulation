#ifndef CHANGEPOINTDIALOG_H
#define CHANGEPOINTDIALOG_H

#include <QDialog>
#include "points.h"

struct ChangePointData
{
    double x;
    double y;
    int mirrorType;
    double radius;
};

class QLabel;
class QDoubleSpinBox;
class QComboBox;
class QPushButton;
class Mirror;

class ChangePointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePointDialog(
        int pointNumber,
        Points point,
        Mirror *mirror,
        bool lastPoint,
        QWidget *parent = nullptr
        );

    ChangePointData getData() const;

private:
    QLabel *pointLabel;
    QDoubleSpinBox *xSpinBox;
    QDoubleSpinBox *ySpinBox;
    QComboBox *mirrorType;
    QDoubleSpinBox *radiusSpinBox;
    QPushButton *cancelButton;
    QPushButton *changeButton;

    Mirror *mirror_;
    ChangePointData data_;

private slots:
    void changeMirrorType();
    void applyChanges();
};

#endif // CHANGEPOINTDIALOG_H
