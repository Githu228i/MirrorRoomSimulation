#ifndef DARKTHEME_H
#define DARKTHEME_H

#include <QString>
#include <QColor>

#include "constants.h"

// ============================================================
//  Стиль главного окна.
//
//  Все цвета и размеры виджетов живут в constants.h
//  (namespace Theme). Здесь только собирается QSS-строка:
//  правила привязаны к именам виджетов, поэтому на диалоги
//  и пузыри чата не влияют.
// ============================================================
namespace Theme {

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

QFrame#line, QFrame#line_2 { background: @BORDER@; border: 1px solid @BORDER@; }

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
    s.replace("@TEXTDIM@", QColor(0x5c, 0x62, 0x6b).name());   // приглушённый цвет отключённых элементов
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

// Выпадающий список QComboBox в диалогах (тёмный, в цвет поля ввода диалога).
// Сам список - отдельное окно, без этого правила оно рисуется системным белым фоном.
inline QString DialogComboPopupStyle()
{
    return QStringLiteral(R"(
QComboBox QAbstractItemView {
    background-color: #303134;
    color: white;
    border: 1px solid #5f6368;
    outline: 0;
    selection-background-color: #4a6fa5;
    selection-color: white;
}
QComboBox QAbstractItemView::item {
    min-height: 26px;
    padding-left: 6px;
}
QComboBox QAbstractItemView::item:hover,
QComboBox QAbstractItemView::item:selected {
    background-color: #4a6fa5;
    color: white;
}
)");
}

} // namespace Theme

#endif // DARKTHEME_H
