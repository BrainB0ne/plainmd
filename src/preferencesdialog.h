#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
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

private slots:
    void onChooseFont();

private:
    QPushButton *m_fontButton = nullptr;
    QLabel *m_fontLabel = nullptr;
    QCheckBox *m_previewCheck = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QFont m_currentFont;
};

#endif // PREFERENCESDIALOG_H
