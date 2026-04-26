/*
 * PlainMD
 * Copyright (C) 2026 BrainByteZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
    bool previewExternalImages() const;
    bool keepRecentFiles() const;
    bool keepRecentFolders() const;
    QString externalEditor() const;
    bool useNerdFontForEmoji() const;
    QFont printEmojiFont() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onChooseFont();
    void onBrowseExternalEditor();
    void onChoosePrintEmojiFont();

private:
    QPushButton *m_fontButton = nullptr;
    QLabel *m_fontLabel = nullptr;
    QLineEdit *m_externalEditorEdit = nullptr;
    QCheckBox *m_previewCheck = nullptr;
    QCheckBox *m_keepRecentCheck = nullptr;
    QCheckBox *m_keepRecentFoldersCheck = nullptr;
    QCheckBox *m_useNerdFontCheck = nullptr;
    QPushButton *m_emojiFontButton = nullptr;
    QLabel *m_emojiFontLabel = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    QFont m_currentFont;
    QFont m_emojiFont;
};

#endif // PREFERENCESDIALOG_H
