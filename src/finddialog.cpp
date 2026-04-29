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

FindDialog::FindDialog(QTextEdit *editor, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
    , m_editor(editor)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Connect return pressed in search field to find next
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &FindDialog::on_findNextButton_clicked);
}

FindDialog::~FindDialog()
{
    delete ui;
}

QString FindDialog::searchText() const
{
    return ui->searchEdit->text();
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

bool FindDialog::performFind(bool fromStart)
{
    if (!m_editor) return false;

    QString text = ui->searchEdit->text();
    if (text.isEmpty()) return false;

    // Emit signal to notify MainWindow of search text (for F3 support)
    emit searchPerformed(text);

    QTextDocument::FindFlags flags;
    if (ui->caseSensitiveCheck->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeWordCheck->isChecked())
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

    return found;
}

void FindDialog::updateStatus(bool found)
{
    if (!found) {
        ui->statusLabel->setText(tr("Text not found."));
    } else {
        ui->statusLabel->clear();
    }
}
