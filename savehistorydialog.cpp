#include "savehistorydialog.h"

#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

SaveHistoryDialog::SaveHistoryDialog(int decimals,
                                     int maxDecimals,
                                     double sampleX,
                                     double sampleY,
                                     QWidget *parent)
    : QDialog(parent), decimals_(decimals), sampleX_(sampleX), sampleY_(sampleY)
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

        QSpinBox {
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

    titleLabel = new QLabel("Сохранение истории отражений", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    hintLabel = new QLabel(
        QString("Сколько знаков после запятой сохранять в координатах "
                "(0 - целые числа). Максимум %1 - предел точности типа, "
                "в котором хранятся координаты.").arg(maxDecimals),
        this
        );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);

    QLabel *decimalsLabel = new QLabel("Знаков после запятой:", this);

    decimalsSpinBox = new QSpinBox(this);
    decimalsSpinBox->setRange(0, maxDecimals);
    decimalsSpinBox->setValue(qBound(0, decimals, maxDecimals));

    QHBoxLayout *decimalsLayout = new QHBoxLayout;
    decimalsLayout->addWidget(decimalsLabel);
    decimalsLayout->addWidget(decimalsSpinBox);

    previewLabel = new QLabel(this);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setWordWrap(true);
    previewLabel->setStyleSheet("color: #9aa0a6;");

    cancelButton = new QPushButton("Отмена", this);
    okButton = new QPushButton("Выбрать файл", this);

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
    mainLayout->addLayout(decimalsLayout);
    mainLayout->addWidget(previewLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    connect(decimalsSpinBox, &QSpinBox::valueChanged,
            this, &SaveHistoryDialog::updatePreview);

    connect(cancelButton, &QPushButton::clicked,
            this, &QDialog::reject);

    connect(okButton, &QPushButton::clicked,
            this, &SaveHistoryDialog::applyChanges);

    updatePreview();
}

// Пример того, как будет записана точка
void SaveHistoryDialog::updatePreview()
{
    const int d = decimalsSpinBox->value();

    previewLabel->setText(
        QString("Пример: (%1; %2)")
            .arg(QString::number(sampleX_, 'f', d),
                 QString::number(sampleY_, 'f', d)));
}

void SaveHistoryDialog::applyChanges()
{
    decimals_ = decimalsSpinBox->value();
    accept();
}

int SaveHistoryDialog::getDecimals() const
{
    return decimals_;
}
