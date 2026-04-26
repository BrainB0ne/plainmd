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

#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class QTextEdit;

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QTextEdit *editor, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onFind();
    void onFindNext();
    void onCloseClicked();

private:
    bool performFind(bool fromStart);
    void updateStatus(bool found);

    QTextEdit *m_editor = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QCheckBox *m_caseSensitiveCheck = nullptr;
    QCheckBox *m_wholeWordCheck = nullptr;
    QPushButton *m_findButton = nullptr;
    QPushButton *m_findNextButton = nullptr;
    QPushButton *m_closeButton = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // FINDDIALOG_H
