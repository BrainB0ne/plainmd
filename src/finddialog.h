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
