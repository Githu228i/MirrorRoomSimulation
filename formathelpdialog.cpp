#include "formathelpdialog.h"
#include "constants.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace {
QString Num(double v, int decimals = 0)
{
    return QString::number(v, 'f', decimals);
}
}

FormatHelpDialog::FormatHelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(DialogSizes::FormatHelpWidth, DialogSizes::FormatHelpHeight);

    setStyleSheet(R"(
        QDialog {
            background-color: #202124;
            border: 1px solid #3c4043;
            border-radius: 15px;
        }

        QLabel {
            color: white;
            font-size: 14px;
        }

        QPushButton {
            background-color: #3c4043;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 9px 30px;
            font-size: 14px;
        }

        QPushButton:hover {
            background-color: #505357;
        }

        QPushButton:pressed {
            background-color: #303134;
        }
    )");

    titleLabel = new QLabel("Формат входного txt-файла", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold;");

    const QString html =
        QString(
            "<p><b>Углы комнаты</b><br>"
            "Строка <b>«Углы комнаты:»</b> (регистр и пробелы перед двоеточием не важны), а сразу "
            "под ней - от %1 до %2 строк подряд. В каждой строке ровно два числа: координаты "
            "<b>x</b> и <b>y</b> вершины (дробная часть - через точку или запятую, минус не "
            "допускается). Рабочая область: x от %3 до %4, y от %5 до %6. Вершины - не ближе "
            "%7 пикселей друг к другу. Комната замыкается сама: последняя точка соединяется с первой.</p>"

            "<p><b>Тип стены</b> (необязательно)<br>"
            "После координат можно дописать <b>«тип: вогнутое(R)»</b> или <b>«тип: выпуклое(R)»</b>, "
            "где R - радиус кривизны. Тип относится к стене, идущей от этой вершины к следующей. "
            "Без записи типа стена плоская. R не меньше половины длины стены и не больше %8.</p>"

            "<p><b>Луч</b> (необязательно)<br>"
            "Строка <b>«Луч:»</b>, а сразу под ней одна строка: <b>x y</b> или <b>x y угол</b>. "
            "Точка должна лежать на стене (допуск %9 пикселей) и быть не ближе %10 пикселей к вершине. "
            "Угол - между стеной и лучом, в градусах, от %11 до %12; если не указан, берётся %13. "
            "Если луч не подошёл, комната всё равно загрузится, а источник нужно будет выбрать "
            "мышью.</p>"

            "<p><b>Пример</b></p>"
            "<pre style='color:#8ab4f8; font-size:14px;'>"
            "Углы комнаты:\n"
            "400 100\n"
            "600 100\n"
            "700 300 тип: вогнутое(300)\n"
            "600 500\n"
            "400 500 тип: выпуклое(250)\n"
            "Луч:\n"
            "500 100 60"
            "</pre>"

            "<p style='color:#9aa0a6;'>Остальные строки файла игнорируются. Если в файле есть "
            "ошибка, комната не изменится, а причины будут показаны в служебном окне.</p>")
            .arg(Num(Limits::MinWalls), Num(Limits::MaxWalls),
                 Num(Limits::MinX), Num(Limits::MaxX),
                 Num(Limits::MinY), Num(Limits::MaxY))
            .arg(Num(Limits::MinPointDist), Num(Limits::MaxMirrorRadius))
            .arg(Num(Limits::BeamWallTolerance), Num(Limits::MinBeamVertexDist))
            .arg(Num(BeamAngle::Min, BeamAngle::Decimals), Num(BeamAngle::Max, BeamAngle::Decimals))
            .arg(Num(Limits::DefaultBeamAngle));

    bodyLabel = new QLabel(html, this);
    bodyLabel->setTextFormat(Qt::RichText);
    bodyLabel->setWordWrap(true);
    bodyLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    closeButton = new QPushButton("Закрыть", this);
    closeButton->setAutoDefault(true);
    closeButton->setDefault(true);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 25, 30, 25);
    mainLayout->setSpacing(15);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(bodyLabel, 1);
    mainLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}
