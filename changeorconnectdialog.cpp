#include "changeorconnectdialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
constexpr int DialogWidth  = 480;   // под три кнопки: «Изменить», «Подключить», «Удалить»
constexpr int DialogHeight = 200;
}

ChangeOrConnectDialog::ChangeOrConnectDialog(QWidget *parent, bool canChange, bool canConnect)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(DialogWidth, DialogHeight);

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

        QPushButton:disabled,
        QPushButton:disabled:hover {
            background-color: #252525;
            color: #777777;
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
    button3 = new QPushButton("Удалить", this);

    // Действие, невозможное для этой точки, гасим
    button1->setEnabled(canChange);
    button2->setEnabled(canConnect);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch();
    buttonLayout->addWidget(button1);
    buttonLayout->addWidget(button2);
    buttonLayout->addWidget(button3);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(20);

    mainLayout->addWidget(textLabel);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    connect(button1, &QPushButton::clicked,
            this, &QDialog::accept);

    // "Подключить" и "Удалить" возвращают свои коды, чтобы отличить их от отмены (Esc = Rejected)
    connect(button2, &QPushButton::clicked,
            this, [this]() { done(ConnectCode); });

    connect(button3, &QPushButton::clicked,
            this, [this]() { done(DeleteCode); });
}
