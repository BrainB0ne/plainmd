#include "finddialog.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTextDocument>

FindDialog::FindDialog(QTextEdit *editor, QWidget *parent)
    : QDialog(parent)
    , m_editor(editor)
{
    setWindowTitle(tr("Find"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(360, 160);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search row
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(tr("Find what:"), this));
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setClearButtonEnabled(true);
    searchLayout->addWidget(m_searchEdit);
    mainLayout->addLayout(searchLayout);

    // Options row
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    m_caseSensitiveCheck = new QCheckBox(tr("Case sensitive"), this);
    optionsLayout->addWidget(m_caseSensitiveCheck);
    m_wholeWordCheck = new QCheckBox(tr("Whole words only"), this);
    optionsLayout->addWidget(m_wholeWordCheck);
    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: #e74c3c; }");
    mainLayout->addWidget(m_statusLabel);

    // Buttons row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_findButton = new QPushButton(tr("&Find"), this);
    m_findButton->setDefault(true);
    connect(m_findButton, &QPushButton::clicked, this, &FindDialog::onFind);
    buttonLayout->addWidget(m_findButton);

    m_findNextButton = new QPushButton(tr("Find &Next"), this);
    connect(m_findNextButton, &QPushButton::clicked, this, &FindDialog::onFindNext);
    buttonLayout->addWidget(m_findNextButton);

    m_closeButton = new QPushButton(tr("Close"), this);
    connect(m_closeButton, &QPushButton::clicked, this, &FindDialog::onCloseClicked);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    // Shortcuts
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindDialog::onFindNext);
}

void FindDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_statusLabel->clear();
    if (m_editor && !m_editor->textCursor().selectedText().isEmpty()) {
        m_searchEdit->setText(m_editor->textCursor().selectedText());
    }
    m_searchEdit->selectAll();
    m_searchEdit->setFocus();
}

void FindDialog::onFind()
{
    m_statusLabel->clear();
    bool found = performFind(true);
    updateStatus(found);
}

void FindDialog::onFindNext()
{
    m_statusLabel->clear();
    bool found = performFind(false);
    updateStatus(found);
}

void FindDialog::onCloseClicked()
{
    hide();
}

bool FindDialog::performFind(bool fromStart)
{
    if (!m_editor) return false;

    QString text = m_searchEdit->text();
    if (text.isEmpty()) return false;

    QTextDocument::FindFlags flags;
    if (m_caseSensitiveCheck->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWordCheck->isChecked())
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
        m_statusLabel->setText(tr("Text not found."));
    } else {
        m_statusLabel->clear();
    }
}
