#ifndef DARKTHEME_H
#define DARKTHEME_H

#include <QColor>
#include <QString>

// Единая тёмная палитра. Цвета для paintEvent и для стилей виджетов берутся отсюда.
namespace Theme {

// Фон окна и рабочей зоны
inline const QColor Window   (0x1b, 0x1d, 0x21);  // фон окна
inline const QColor Canvas   (0x12, 0x14, 0x17);  // фон рабочей зоны (комната)
inline const QColor CanvasBorder(0x34, 0x38, 0x40); // рамка рабочей зоны
// Элементы управления
inline const QColor Panel    (0x26, 0x29, 0x2e);  // кнопки, поле ввода
inline const QColor PanelHover(0x30, 0x34, 0x3b);
inline const QColor PanelPress(0x1f, 0x22, 0x26);
inline const QColor Border   (0x3a, 0x3e, 0x46);
inline const QColor Text     (0xe6, 0xe8, 0xeb);
inline const QColor TextMuted(0x8b, 0x91, 0x9b);
inline const QColor Accent   (0x4d, 0xa3, 0xff);  // ползунки, галочка, фокус
// Рисование комнаты (на тёмном фоне чёрное не видно)
inline const QColor Wall     (0xe6, 0xe8, 0xeb);  // стены и вершины
inline const QColor Source   (0xff, 0x52, 0x52);  // источник луча (красный)
inline const QColor Trace    (0xff, 0xd5, 0x4a);  // пройденные линии (жёлтый)
inline const QColor Reflect  (0xff, 0x9f, 0x1a);  // точка последнего отражения (оранжевый)

// Размеры элементов стиля (px)
inline constexpr int ButtonRadius   = 10;
inline constexpr int CheckSize      = 26;
inline constexpr int SliderGroove   = 6;
inline constexpr int SliderHandle   = 20;
inline constexpr int ScrollBarWidth = 10;

// Стиль главного окна. Все правила привязаны к именам виджетов,
// поэтому на диалоги и пузыри чата не влияют.
inline QString MainStyle()
{
    QString s = QStringLiteral(R"(
/* Фон окна и комнату рисует MainWindow::paintEvent, поэтому центральный виджет прозрачный */
#centralwidget { background: transparent; }

#label, #label_2, #label_3, #checkBox { color: @TEXT@; background: transparent; }
#label:disabled, #label_2:disabled, #label_3:disabled, #checkBox:disabled { color: @MUTED@; }

QPushButton[objectName^="pushButton"] {
    background: @PANEL@; color: @TEXT@;
    border: 1px solid @BORDER@; border-radius: @BTNR@px;
}
QPushButton[objectName^="pushButton"]:hover   { background: @HOVER@; border-color: @ACCENT@; }
QPushButton[objectName^="pushButton"]:pressed { background: @PRESS@; }
QPushButton[objectName^="pushButton"]:disabled { background: @WINDOW@; color: @MUTED@; border-color: @PANEL@; }

#checkBox::indicator {
    width: @CHK@px; height: @CHK@px;
    border: 2px solid @BORDER@; border-radius: 6px; background: @PANEL@;
}
#checkBox::indicator:hover   { border-color: @ACCENT@; }
#checkBox::indicator:checked { background: @ACCENT@; border-color: @ACCENT@; }
#checkBox::indicator:disabled { border-color: @PANEL@; background: @WINDOW@; }

QSlider[objectName^="horizontalSlider"]::groove:horizontal {
    height: @GRV@px; background: @BORDER@; border-radius: 3px;
}
QSlider[objectName^="horizontalSlider"]::sub-page:horizontal {
    background: @ACCENT@; border-radius: 3px;
}
QSlider[objectName^="horizontalSlider"]::handle:horizontal {
    background: @TEXT@; border: 2px solid @ACCENT@;
    width: @HND@px; height: @HND@px; margin: -8px 0; border-radius: 12px;
}
QSlider[objectName^="horizontalSlider"]::handle:horizontal:hover { background: @ACCENT@; }
QSlider[objectName^="horizontalSlider"]::sub-page:horizontal:disabled { background: @TEXTDIM@; }
QSlider[objectName^="horizontalSlider"]::handle:horizontal:disabled {
    background: @PANEL@; border-color: @TEXTDIM@;
}

#textEdit {
    background: @PANEL@; color: @TEXT@;
    border: 1px solid @BORDER@; border-radius: 6px;
    selection-background-color: @ACCENT@; selection-color: @WINDOW@;
}
#textEdit:focus { border-color: @ACCENT@; }

QFrame#line, QFrame#line_2 { background: @BORDER@; color: @BORDER@; border: none; }

#chatScrollArea QScrollBar:vertical { background: transparent; width: @SBW@px; margin: 0; }
#chatScrollArea QScrollBar::handle:vertical { background: @BORDER@; border-radius: 5px; min-height: 30px; }
#chatScrollArea QScrollBar::handle:vertical:hover { background: @TEXTDIM@; }
#chatScrollArea QScrollBar::add-line:vertical, #chatScrollArea QScrollBar::sub-line:vertical { height: 0; }
#chatScrollArea QScrollBar::add-page:vertical, #chatScrollArea QScrollBar::sub-page:vertical { background: transparent; }
)");

    s.replace("@WINDOW@",  Window.name());
    s.replace("@PANEL@",   Panel.name());
    s.replace("@HOVER@",   PanelHover.name());
    s.replace("@PRESS@",   PanelPress.name());
    s.replace("@BORDER@",  Border.name());
    s.replace("@TEXTDIM@", QColor(0x5a, 0x60, 0x6a).name());
    s.replace("@TEXT@",    Text.name());
    s.replace("@MUTED@",   TextMuted.name());
    s.replace("@ACCENT@",  Accent.name());
    s.replace("@BTNR@",    QString::number(ButtonRadius));
    s.replace("@CHK@",     QString::number(CheckSize));
    s.replace("@GRV@",     QString::number(SliderGroove));
    s.replace("@HND@",     QString::number(SliderHandle));
    s.replace("@SBW@",     QString::number(ScrollBarWidth));
    return s;
}

} // namespace Theme

#endif // DARKTHEME_H
