#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    void loadSettings();
    void saveSettings();

    QString fontFamily() const;
    int fontSize() const;
    QString codeBlockFontFamily() const;
    int codeBlockFontSize() const;
    bool previewExternalImages() const;
    bool keepRecentFiles() const;
    QString externalEditor() const;

private slots:
    void onChooseFont();
    void onChooseCodeBlockFont();
    void onBrowseExternalEditor();

private:
    QPushButton *m_fontButton = nullptr;
    QLabel *m_fontLabel = nullptr;
    QPushButton *m_codeBlockFontButton = nullptr;
    QLabel *m_codeBlockFontLabel = nullptr;
    QLineEdit *m_externalEditorEdit = nullptr;
    QPushButton *m_externalEditorBrowseButton = nullptr;
    QCheckBox *m_previewCheck = nullptr;
    QCheckBox *m_keepRecentCheck = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QFont m_currentFont;
    QFont m_codeBlockFont;
};

#endif // PREFERENCESDIALOG_H
