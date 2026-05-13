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

#include "finddialog.h"
#include "ui_finddialog.h"

#include <QTextDocument>
#include <QTextEdit>
#include <QRegularExpression>

FindDialog::FindDialog(QTextEdit *editor, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
    , m_editor(editor)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Connect return pressed in search field to find next
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &FindDialog::on_findNextButton_clicked);

    // Disable whole-word checkbox when regex is active (regex uses \b for word boundaries)
    connect(ui->regexCheck, &QCheckBox::toggled, this, [this](bool checked) {
        ui->wholeWordCheck->setEnabled(!checked);
        if (checked) ui->wholeWordCheck->setChecked(false);
    });
}

FindDialog::~FindDialog()
{
    delete ui;
}

QString FindDialog::searchText() const
{
    return ui->searchEdit->text();
}

bool FindDialog::caseSensitive() const
{
    return ui->caseSensitiveCheck->isChecked();
}

bool FindDialog::wholeWords() const
{
    return ui->wholeWordCheck->isChecked();
}

bool FindDialog::regex() const
{
    return ui->regexCheck->isChecked();
}

void FindDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    ui->statusLabel->clear();
    if (m_editor && !m_editor->textCursor().selectedText().isEmpty()) {
        ui->searchEdit->setText(m_editor->textCursor().selectedText());
    }
    ui->searchEdit->selectAll();
    ui->searchEdit->setFocus();
}

void FindDialog::on_findButton_clicked()
{
    ui->statusLabel->clear();
    bool found = performFind(true);
    updateStatus(found);
}

void FindDialog::on_findNextButton_clicked()
{
    ui->statusLabel->clear();
    bool found = performFind(false);
    updateStatus(found);
}

void FindDialog::on_closeButton_clicked()
{
    hide();
}

bool FindDialog::findNext()
{
    // Perform search from current cursor position (for F3 from MainWindow)
    return performFind(false);
}

bool FindDialog::performFind(bool fromStart)
{
    if (!m_editor) return false;

    QString text = ui->searchEdit->text();
    if (text.isEmpty()) return false;

    // Emit signal to notify MainWindow of search text (for F3 support)
    emit searchPerformed(text);

    bool caseSensitive = ui->caseSensitiveCheck->isChecked();
    bool wholeWords = ui->wholeWordCheck->isChecked();
    bool useRegex = ui->regexCheck->isChecked();

    if (useRegex) {
        return performRegexFind(text, caseSensitive, fromStart);
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (wholeWords)
        flags |= QTextDocument::FindWholeWords;

    if (fromStart) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.setPosition(0);
        m_editor->setTextCursor(cursor);
    }

    bool found = m_editor->find(text, flags);

    if (!found) {
        // Wrap around: try from the beginning
        QTextCursor cursor = m_editor->textCursor();
        cursor.setPosition(0);
        m_editor->setTextCursor(cursor);
        found = m_editor->find(text, flags);
    }

    // Set focus so the selection is shown in color (not grey)
    if (found) {
        m_editor->setFocus();
    }

    return found;
}

bool FindDialog::performRegexFind(const QString &pattern, bool caseSensitive, bool fromStart)
{
    if (!m_editor) return false;

    QString documentText = m_editor->toPlainText();
    QRegularExpression::PatternOptions options = QRegularExpression::MultilineOption;
    if (!caseSensitive)
        options |= QRegularExpression::CaseInsensitiveOption;

    QRegularExpression regex(pattern, options);
    if (!regex.isValid()) {
        ui->statusLabel->setText(tr("Invalid regular expression: %1").arg(regex.errorString()));
        return false;
    }

    int startPos = fromStart ? 0 : m_editor->textCursor().position();

    // Search from startPos to end
    QRegularExpressionMatch match = regex.match(documentText, startPos);

    if (!match.hasMatch() || match.capturedStart() < startPos) {
        // Wrap around: search from beginning
        match = regex.match(documentText, 0);
    }

    if (match.hasMatch()) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.setPosition(match.capturedStart());
        cursor.setPosition(match.capturedEnd(), QTextCursor::KeepAnchor);
        m_editor->setTextCursor(cursor);
        m_editor->setFocus();
        return true;
    }

    return false;
}

void FindDialog::updateStatus(bool found)
{
    if (found) {
        ui->statusLabel->clear();
    } else if (ui->statusLabel->text().isEmpty()) {
        // Only show "not found" if no error message is already displayed
        // (e.g. invalid regex error set by performRegexFind)
        ui->statusLabel->setText(tr("Text not found."));
    }
}
