#include "changeorconnectdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

ChangeOrConnectDialog::ChangeOrConnectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(400, 200);

    setStyleSheet(R"(
        QDialog {
            background-color: #202124;
            border: 1px solid #3c4043;
            border-radius: 15px;
        }

        QLabel {
            color: white;
            font-size: 17px;
        }

        QPushButton {
            background-color: #3c4043;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 25px;
            font-size: 14px;
        }

        QPushButton:hover {
            background-color: #505357;
        }

        QPushButton:pressed {
            background-color: #303134;
        }
    )");

    textLabel = new QLabel(
        "ТУТ БУДЕТ ТВОЙ ТЕКСТ",
        this
        );

    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);

    button1 = new QPushButton("Изменить", this);
    button2 = new QPushButton("Подключить", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch();
    buttonLayout->addWidget(button1);
    buttonLayout->addWidget(button2);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(20);

    mainLayout->addWidget(textLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(button1, &QPushButton::clicked,
            this, &QDialog::accept);

    connect(button2, &QPushButton::clicked,
            this, &QDialog::reject);
}
