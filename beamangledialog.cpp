#include "beamangledialog.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

BeamAngleDialog::BeamAngleDialog(QWidget *parent, double initialAngle)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(450, 280);

    setStyleSheet(R"(
        QDialog {
            background-color: #202124;
            border: 1px solid #3c4043;
            border-radius: 15px;
        }

        QLabel {
            color: white;
            font-size: 15px;
        }

        QDoubleSpinBox {
            background-color: #303134;
            color: white;
            border: 1px solid #5f6368;
            border-radius: 6px;
            padding: 5px;
        }

        QPushButton {
            background-color: #3c4043;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 9px 20px;
            font-size: 14px;
        }

        QPushButton:hover {
            background-color: #505357;
        }

        QPushButton:pressed {
            background-color: #303134;
        }
    )");

    titleLabel = new QLabel("Направление луча", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    hintLabel = new QLabel(
        "Угол между стеной и лучом (в сторону комнаты). "
        "90° - перпендикулярно стене. "
        "Для кривой стены берётся касательная в точке луча.",
        this
        );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);

    QLabel *angleLabel = new QLabel("Угол:", this);

    angleSpinBox = new QDoubleSpinBox(this);
    angleSpinBox->setRange(0.1, 179.9);
    angleSpinBox->setDecimals(2);
    angleSpinBox->setSuffix("°");
    angleSpinBox->setValue(initialAngle);

    QHBoxLayout *angleLayout = new QHBoxLayout;
    angleLayout->addWidget(angleLabel);
    angleLayout->addWidget(angleSpinBox);

    cancelButton = new QPushButton("Отмена", this);
    okButton = new QPushButton("Задать", this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(15);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(hintLabel);
    mainLayout->addLayout(angleLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    connect(cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    connect(okButton, &QPushButton::clicked,
            this, &BeamAngleDialog::applyChanges);
}

void BeamAngleDialog::applyChanges()
{
    angle_ = angleSpinBox->value();
    accept();
}

double BeamAngleDialog::getAngle() const
{
    return angle_;
}
