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

namespace Ui {
class FindDialog;
}

class QTextEdit;

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QTextEdit *editor, QWidget *parent = nullptr);
    ~FindDialog();

    QString searchText() const;

protected:
    void showEvent(QShowEvent *event) override;

signals:
    void searchPerformed(const QString &text);

private slots:
    void on_findButton_clicked();
    void on_findNextButton_clicked();
    void on_closeButton_clicked();

private:
    bool performFind(bool fromStart);
    void updateStatus(bool found);

    Ui::FindDialog *ui;
    QTextEdit *m_editor = nullptr;
};

#endif // FINDDIALOG_H
