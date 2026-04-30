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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QSettings>
#include <QApplication>
#include <QFontDialog>
#include <QFileDialog>
#include <QDir>
#include <QMouseEvent>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    resize(400, 300);

    // Install event filter on external editor field to handle clicks
    ui->externalEditorEdit->installEventFilter(this);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());

#ifdef Q_OS_LINUX
    const QString defaultFontFamily = QStringLiteral("DejaVu Sans");
#else
    const QString defaultFontFamily = QStringLiteral("Segoe UI");
#endif

    QString family = settings.value("editor/fontFamily", defaultFontFamily).toString();
    int size = settings.value("editor/fontSize", 11).toInt();
    m_currentFont = QFont(family);
    m_currentFont.setPointSize(size);
    ui->currentFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_currentFont.family()).arg(m_currentFont.pointSize()));

    ui->externalEditorEdit->setText(QDir::toNativeSeparators(settings.value("editor/externalEditor").toString()));

    // Load emoji print font
#ifdef Q_OS_LINUX
    QString defaultEmojiFont = QStringLiteral("Noto Sans");
#else
    QString defaultEmojiFont = QStringLiteral("Segoe UI");
#endif
    QString emojiFamily = settings.value("editor/printEmojiFont", defaultEmojiFont).toString();
    int emojiSize = settings.value("editor/printEmojiFontSize", 11).toInt();
    m_emojiFont = QFont(emojiFamily);
    m_emojiFont.setPointSize(emojiSize);
    ui->currentEmojiFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_emojiFont.family()).arg(m_emojiFont.pointSize()));

    ui->useNerdFontCheck->setChecked(settings.value("editor/useNerdFontForEmoji", false).toBool());

    ui->previewCheck->setChecked(settings.value("privacy/previewExternalImages", true).toBool());
    ui->keepRecentCheck->setChecked(settings.value("privacy/keepRecentFiles", true).toBool());
    ui->keepRecentFoldersCheck->setChecked(settings.value("privacy/keepRecentFolders", true).toBool());
    ui->rememberLastFolderCheck->setChecked(settings.value("privacy/rememberLastFolder", true).toBool());

    // Load window title format (0 = filename only, 1 = full path)
    ui->windowTitleCombo->setCurrentIndex(settings.value("view/windowTitleFormat", 0).toInt());
}

void PreferencesDialog::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("editor/fontFamily", m_currentFont.family());
    settings.setValue("editor/fontSize", m_currentFont.pointSize());
    settings.setValue("editor/externalEditor", QDir::fromNativeSeparators(ui->externalEditorEdit->text()));
    settings.setValue("editor/printEmojiFont", m_emojiFont.family());
    settings.setValue("editor/printEmojiFontSize", m_emojiFont.pointSize());
    settings.setValue("editor/useNerdFontForEmoji", ui->useNerdFontCheck->isChecked());
    settings.setValue("privacy/previewExternalImages", ui->previewCheck->isChecked());
    settings.setValue("privacy/keepRecentFiles", ui->keepRecentCheck->isChecked());
    settings.setValue("privacy/keepRecentFolders", ui->keepRecentFoldersCheck->isChecked());
    settings.setValue("privacy/rememberLastFolder", ui->rememberLastFolderCheck->isChecked());
    settings.setValue("view/windowTitleFormat", ui->windowTitleCombo->currentIndex());
}

QString PreferencesDialog::fontFamily() const
{
    return m_currentFont.family();
}

int PreferencesDialog::fontSize() const
{
    return m_currentFont.pointSize();
}

bool PreferencesDialog::previewExternalImages() const
{
    return ui->previewCheck->isChecked();
}

bool PreferencesDialog::keepRecentFiles() const
{
    return ui->keepRecentCheck->isChecked();
}

bool PreferencesDialog::keepRecentFolders() const
{
    return ui->keepRecentFoldersCheck->isChecked();
}

bool PreferencesDialog::rememberLastFolder() const
{
    return ui->rememberLastFolderCheck->isChecked();
}

QString PreferencesDialog::externalEditor() const
{
    return ui->externalEditorEdit->text();
}

QFont PreferencesDialog::printEmojiFont() const
{
    return m_emojiFont;
}

bool PreferencesDialog::useNerdFontForEmoji() const
{
    return ui->useNerdFontCheck->isChecked();
}

void PreferencesDialog::on_fontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_currentFont, this);
    if (ok) {
        m_currentFont = font;
        ui->currentFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_currentFont.family()).arg(m_currentFont.pointSize()));
    }
}

void PreferencesDialog::on_emojiFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_emojiFont, this);
    if (ok) {
        m_emojiFont = font;
        ui->currentEmojiFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_emojiFont.family()).arg(m_emojiFont.pointSize()));
    }
}

void PreferencesDialog::browseExternalEditor()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select External Editor"),
                                                ui->externalEditorEdit->text(),
#ifdef Q_OS_WIN
                                                tr("Executable Files (*.exe);;All Files (*.*)")
#else
                                                tr("All Files (*)")
#endif
                                                );
    if (!path.isEmpty()) {
        ui->externalEditorEdit->setText(QDir::toNativeSeparators(path));
    }
}

int PreferencesDialog::windowTitleFormat() const
{
    return ui->windowTitleCombo->currentIndex();
}

bool PreferencesDialog::eventFilter(QObject *watched, QEvent *event)
{
    // Handle left click on the external editor field (read-only QLineEdit acting as button)
    if (watched == ui->externalEditorEdit && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            browseExternalEditor();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
