#ifndef SAVEHISTORYDIALOG_H
#define SAVEHISTORYDIALOG_H

#include <QDialog>

class QLabel;
class QSpinBox;
class QPushButton;

// Диалог перед сохранением истории: сколько знаков после запятой писать в координатах
class SaveHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    // decimals - значение по умолчанию, maxDecimals - предел типа координат,
    // sampleX/sampleY - пример числа для предпросмотра
    explicit SaveHistoryDialog(int decimals,
                               int maxDecimals,
                               double sampleX,
                               double sampleY,
                               QWidget *parent = nullptr);

    int getDecimals() const;

private:
    QLabel *titleLabel;
    QLabel *hintLabel;
    QLabel *previewLabel;
    QSpinBox *decimalsSpinBox;
    QPushButton *cancelButton;
    QPushButton *okButton;

    int decimals_;
    double sampleX_;
    double sampleY_;

private slots:
    void updatePreview();
    void applyChanges();
};

#endif // SAVEHISTORYDIALOG_H
