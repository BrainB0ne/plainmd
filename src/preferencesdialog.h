#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QEvent>

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
    bool useNerdFontForEmoji() const;
    QFont printEmojiFont() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onChooseFont();
    void onChooseCodeBlockFont();
    void onBrowseExternalEditor();
    void onChoosePrintEmojiFont();

private:
    QPushButton *m_fontButton = nullptr;
    QLabel *m_fontLabel = nullptr;
    QPushButton *m_codeBlockFontButton = nullptr;
    QLabel *m_codeBlockFontLabel = nullptr;
    QLineEdit *m_externalEditorEdit = nullptr;
    QCheckBox *m_previewCheck = nullptr;
    QCheckBox *m_keepRecentCheck = nullptr;
    QCheckBox *m_useNerdFontCheck = nullptr;
    QPushButton *m_emojiFontButton = nullptr;
    QLabel *m_emojiFontLabel = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QFont m_currentFont;
    QFont m_codeBlockFont;
    QFont m_emojiFont;
};

#endif // PREFERENCESDIALOG_H
