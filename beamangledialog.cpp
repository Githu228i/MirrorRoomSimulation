#include "beamangledialog.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QKeyEvent>

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
        QString("Угол между стеной и лучом (в сторону комнаты), от %1° до %2° включительно. "
                "90° - перпендикулярно стене. "
                "Для кривой стены берётся касательная в точке луча.")
            .arg(BeamAngle::Min, 0, 'f', BeamAngle::Decimals)
            .arg(BeamAngle::Max, 0, 'f', BeamAngle::Decimals),
        this
        );
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);

    QLabel *angleLabel = new QLabel("Угол:", this);

    angleSpinBox = new QDoubleSpinBox(this);
    angleSpinBox->setDecimals(BeamAngle::Decimals);   // сначала знаки, потом диапазон
    angleSpinBox->setRange(BeamAngle::Min, BeamAngle::Max);
    angleSpinBox->setSuffix("°");
    angleSpinBox->setValue(initialAngle);

    // Enter в поле обрабатываем сами (см. eventFilter): иначе спинбокс молча
    // подменит недопустимый ввод прежним значением
    // (lineEdit() у QAbstractSpinBox защищён, поэтому ищем поле как дочерний виджет)
    angleEdit = angleSpinBox->findChild<QLineEdit *>();
    if (angleEdit)
        angleEdit->installEventFilter(this);

    QHBoxLayout *angleLayout = new QHBoxLayout;
    angleLayout->addWidget(angleLabel);
    angleLayout->addWidget(angleSpinBox);

    cancelButton = new QPushButton("Отмена", this);
    okButton = new QPushButton("Задать", this);

    // Кнопки не берут фокус: иначе при клике спинбокс теряет фокус и успевает
    // молча заменить недопустимый ввод (например, 0) прежним значением
    cancelButton->setFocusPolicy(Qt::NoFocus);
    okButton->setFocusPolicy(Qt::NoFocus);
    cancelButton->setAutoDefault(false);
    okButton->setAutoDefault(false);

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

bool BeamAngleDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (angleEdit && obj == angleEdit && event->type() == QEvent::KeyPress)
    {
        const int key = static_cast<QKeyEvent *>(event)->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter)
        {
            applyChanges();
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

// Проверяет именно набранный текст (а не последнее "принятое" значение спинбокса).
// Недопустимый угол: диалог остаётся открытым, сообщение об ошибке уходит в чат.
void BeamAngleDialog::applyChanges()
{
    const QString typed = angleSpinBox->cleanText().trimmed();

    QString normalized = typed;
    normalized.replace(',', '.');

    bool ok = false;
    const double value = normalized.toDouble(&ok);

    if (!ok || value < BeamAngle::Min || value > BeamAngle::Max)
    {
        emit invalidAngle(
            QString("Угол должен быть числом от %1° до %2° включительно (введено «%3»). "
                    "Значение не принято: исправьте угол.")
                .arg(BeamAngle::Min, 0, 'f', BeamAngle::Decimals)
                .arg(BeamAngle::Max, 0, 'f', BeamAngle::Decimals)
                .arg(typed.isEmpty() ? QString("пусто") : typed));
        return;
    }

    angle_ = value;
    accept();
}

double BeamAngleDialog::getAngle() const
{
    return angle_;
}
