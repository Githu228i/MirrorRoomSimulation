#include "changepointdialog.h"
#include "mirror.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ChangePointDialog::ChangePointDialog(
    int pointNumber,
    Points point,
    Mirror *mirror,
    bool lastPoint,
    QWidget *parent
    )
    : QDialog(parent), mirror_(mirror)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(450, 330);

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

        QDoubleSpinBox, QComboBox {
            background-color: #303134;
            color: white;
            border: 1px solid #5f6368;
            border-radius: 6px;
            padding: 5px;
        }

        QDoubleSpinBox:disabled,
        QComboBox:disabled {
            color: #777777;
            background-color: #252525;
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

    pointLabel = new QLabel(
        "Точка номер: " + QString::number(pointNumber),
        this
        );

    pointLabel->setAlignment(Qt::AlignCenter);
    pointLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold;"
        );

    QLabel *xLabel = new QLabel("X:", this);
    QLabel *yLabel = new QLabel("Y:", this);

    xSpinBox = new QDoubleSpinBox(this);
    ySpinBox = new QDoubleSpinBox(this);

    xSpinBox->setRange(-1000000.0, 1000000.0);
    ySpinBox->setRange(-1000000.0, 1000000.0);

    xSpinBox->setDecimals(6);
    ySpinBox->setDecimals(6);

    xSpinBox->setValue(point.getX());
    ySpinBox->setValue(point.getY());

    QHBoxLayout *coordinatesLayout = new QHBoxLayout;

    coordinatesLayout->addWidget(xLabel);
    coordinatesLayout->addWidget(xSpinBox);
    coordinatesLayout->addWidget(yLabel);
    coordinatesLayout->addWidget(ySpinBox);

    QLabel *mirrorLabel = new QLabel(
        "Выберите тип зеркала, исходящий из этой точки",
        this
        );

    mirrorLabel->setAlignment(Qt::AlignCenter);
    mirrorLabel->setWordWrap(true);

    mirrorType = new QComboBox(this);

    mirrorType->addItem("Плоское зеркало");
    mirrorType->addItem("Вогнутое зеркало");
    mirrorType->addItem("Выпуклое зеркало");

    QLabel *radiusLabel = new QLabel(
        "Радиус кривизны:",
        this
        );

    radiusSpinBox = new QDoubleSpinBox(this);

    radiusSpinBox->setRange(0.000001, 1000000.0);
    radiusSpinBox->setDecimals(6);
    radiusSpinBox->setEnabled(false);

    QHBoxLayout *radiusLayout = new QHBoxLayout;

    radiusLayout->addWidget(radiusLabel);
    radiusLayout->addWidget(radiusSpinBox);

    cancelButton = new QPushButton("Отмена", this);
    changeButton = new QPushButton("Изменить", this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;

    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(changeButton);
    buttonsLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(15);

    mainLayout->addWidget(pointLabel);
    mainLayout->addLayout(coordinatesLayout);
    mainLayout->addWidget(mirrorLabel);
    mainLayout->addWidget(mirrorType);
    mainLayout->addLayout(radiusLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    // Для последней точки незамкнутого полигона
    // зеркало из неё ещё не существует.
    if (lastPoint)
    {
        mirrorType->setEnabled(false);
        radiusSpinBox->setEnabled(false);
    }

    connect(
        mirrorType,
        &QComboBox::currentIndexChanged,
        this,
        &ChangePointDialog::changeMirrorType
        );

    connect(
        cancelButton,
        &QPushButton::clicked,
        this,
        &QDialog::reject
        );

    connect(
        changeButton,
        &QPushButton::clicked,
        this,
        &ChangePointDialog::applyChanges
        );
}

void ChangePointDialog::changeMirrorType()
{
    radiusSpinBox->setEnabled(mirrorType->currentIndex() != 0);
}

void ChangePointDialog::applyChanges()
{
    data_.x = xSpinBox->value();
    data_.y = ySpinBox->value();
    data_.mirrorType = mirrorType->currentIndex();
    data_.radius = radiusSpinBox->value();

    accept();
}

ChangePointData ChangePointDialog::getData() const
{
    return data_;
}
