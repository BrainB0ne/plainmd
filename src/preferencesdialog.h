#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    void loadSettings();
    void saveSettings();

    QString fontFamily() const;
    int fontSize() const;
    bool previewExternalImages() const;

private:
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_sizeSpin = nullptr;
    QCheckBox *m_previewCheck = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
};

#endif // PREFERENCESDIALOG_H
