#include "preferencesdialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSettings>
#include <QApplication>
#include <QFontDialog>
#include <QFileDialog>
#include <QDir>
#include <QMouseEvent>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Editor group
    QGroupBox *editorGroup = new QGroupBox(tr("Editor"), this);
    QGridLayout *editorLayout = new QGridLayout(editorGroup);

    editorLayout->addWidget(new QLabel(tr("Font:"), this), 0, 0);
    m_fontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_fontButton, &QPushButton::clicked, this, &PreferencesDialog::onChooseFont);
    editorLayout->addWidget(m_fontButton, 0, 1);
    m_fontLabel = new QLabel(this);
    editorLayout->addWidget(m_fontLabel, 0, 2);

    editorLayout->addWidget(new QLabel(tr("Code Font:"), this), 1, 0);
    m_codeBlockFontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_codeBlockFontButton, &QPushButton::clicked, this, &PreferencesDialog::onChooseCodeBlockFont);
    editorLayout->addWidget(m_codeBlockFontButton, 1, 1);
    m_codeBlockFontLabel = new QLabel(this);
    editorLayout->addWidget(m_codeBlockFontLabel, 1, 2);

    // Emoji/Print font (Nerd Font support) - moved up below Code Font
    editorLayout->addWidget(new QLabel(tr("Emoji Print Font:"), this), 2, 0);
    m_emojiFontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_emojiFontButton, &QPushButton::clicked, this, &PreferencesDialog::onChoosePrintEmojiFont);
    editorLayout->addWidget(m_emojiFontButton, 2, 1);
    m_emojiFontLabel = new QLabel(this);
    editorLayout->addWidget(m_emojiFontLabel, 2, 2);
    m_useNerdFontCheck = new QCheckBox(tr("Use for emoji printing"), this);
    editorLayout->addWidget(m_useNerdFontCheck, 3, 0, 1, 3);

    // External editor - clickable read-only field (acts as button)
    editorLayout->addWidget(new QLabel(tr("External Editor:"), this), 6, 0);
    m_externalEditorEdit = new QLineEdit(this);
    m_externalEditorEdit->setPlaceholderText(tr("Click to select editor..."));
    m_externalEditorEdit->setReadOnly(true);
    m_externalEditorEdit->setCursor(Qt::PointingHandCursor);
    // Style it to look clickable like a button
    m_externalEditorEdit->setStyleSheet(QStringLiteral("QLineEdit { background: palette(button); border: 1px solid palette(mid); border-radius: 2px; }"));
    editorLayout->addWidget(m_externalEditorEdit, 6, 1, 1, 2);  // Span 2 columns
    // Install event filter to handle clicks
    m_externalEditorEdit->installEventFilter(this);

    mainLayout->addWidget(editorGroup);

    // Privacy group
    QGroupBox *privacyGroup = new QGroupBox(tr("Privacy"), this);
    QVBoxLayout *privacyLayout = new QVBoxLayout(privacyGroup);
    m_previewCheck = new QCheckBox(tr("Preview external images"), this);
    privacyLayout->addWidget(m_previewCheck);

    m_keepRecentCheck = new QCheckBox(tr("Keep recent files history"), this);
    privacyLayout->addWidget(m_keepRecentCheck);
    mainLayout->addWidget(privacyGroup);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
    mainLayout->addStretch();
}

void PreferencesDialog::loadSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());

#ifdef Q_OS_LINUX
    const QString defaultFontFamily = QStringLiteral("DejaVu Sans");
    const QString defaultCodeBlockFontFamily = QStringLiteral("DejaVu Sans Mono");
#else
    const QString defaultFontFamily = QStringLiteral("Segoe UI");
    const QString defaultCodeBlockFontFamily = QStringLiteral("Consolas");
#endif

    QString family = settings.value("editor/fontFamily", defaultFontFamily).toString();
    int size = settings.value("editor/fontSize", 11).toInt();
    m_currentFont = QFont(family);
    m_currentFont.setPointSize(size);
    m_fontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_currentFont.family()).arg(m_currentFont.pointSize()));

    QString cbFamily = settings.value("editor/codeBlockFontFamily", defaultCodeBlockFontFamily).toString();
    int cbSize = settings.value("editor/codeBlockFontSize", 11).toInt();
    m_codeBlockFont = QFont(cbFamily);
    m_codeBlockFont.setPointSize(cbSize);
    m_codeBlockFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_codeBlockFont.family()).arg(m_codeBlockFont.pointSize()));

    m_externalEditorEdit->setText(QDir::toNativeSeparators(settings.value("editor/externalEditor").toString()));
    
    // Load emoji print font
    QString emojiFamily = settings.value("editor/printEmojiFont", QStringLiteral("Segoe UI")).toString();
    int emojiSize = settings.value("editor/printEmojiFontSize", 11).toInt();
    m_emojiFont = QFont(emojiFamily);
    m_emojiFont.setPointSize(emojiSize);
    m_emojiFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_emojiFont.family()).arg(m_emojiFont.pointSize()));
    
    m_useNerdFontCheck->setChecked(settings.value("editor/useNerdFontForEmoji", false).toBool());

    m_previewCheck->setChecked(settings.value("privacy/previewExternalImages", true).toBool());
    m_keepRecentCheck->setChecked(settings.value("privacy/keepRecentFiles", true).toBool());
}

void PreferencesDialog::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("editor/fontFamily", m_currentFont.family());
    settings.setValue("editor/fontSize", m_currentFont.pointSize());
    settings.setValue("editor/codeBlockFontFamily", m_codeBlockFont.family());
    settings.setValue("editor/codeBlockFontSize", m_codeBlockFont.pointSize());
    settings.setValue("editor/externalEditor", QDir::fromNativeSeparators(m_externalEditorEdit->text()));
    settings.setValue("editor/printEmojiFont", m_emojiFont.family());
    settings.setValue("editor/printEmojiFontSize", m_emojiFont.pointSize());
    settings.setValue("editor/useNerdFontForEmoji", m_useNerdFontCheck->isChecked());
    settings.setValue("privacy/previewExternalImages", m_previewCheck->isChecked());
    settings.setValue("privacy/keepRecentFiles", m_keepRecentCheck->isChecked());
}

QString PreferencesDialog::fontFamily() const
{
    return m_currentFont.family();
}

int PreferencesDialog::fontSize() const
{
    return m_currentFont.pointSize();
}

QString PreferencesDialog::codeBlockFontFamily() const
{
    return m_codeBlockFont.family();
}

int PreferencesDialog::codeBlockFontSize() const
{
    return m_codeBlockFont.pointSize();
}

bool PreferencesDialog::previewExternalImages() const
{
    return m_previewCheck->isChecked();
}

bool PreferencesDialog::keepRecentFiles() const
{
    return m_keepRecentCheck->isChecked();
}

QString PreferencesDialog::externalEditor() const
{
    return m_externalEditorEdit->text();
}

QFont PreferencesDialog::printEmojiFont() const
{
    return m_emojiFont;
}

bool PreferencesDialog::useNerdFontForEmoji() const
{
    return m_useNerdFontCheck->isChecked();
}

void PreferencesDialog::onChooseFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_currentFont, this);
    if (ok) {
        m_currentFont = font;
        m_fontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_currentFont.family()).arg(m_currentFont.pointSize()));
    }
}

void PreferencesDialog::onChooseCodeBlockFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_codeBlockFont, this);
    if (ok) {
        m_codeBlockFont = font;
        m_codeBlockFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_codeBlockFont.family()).arg(m_codeBlockFont.pointSize()));
    }
}

void PreferencesDialog::onChoosePrintEmojiFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_emojiFont, this);
    if (ok) {
        m_emojiFont = font;
        m_emojiFontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_emojiFont.family()).arg(m_emojiFont.pointSize()));
    }
}

void PreferencesDialog::onBrowseExternalEditor()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Select External Editor"),
                                                m_externalEditorEdit->text(),
#ifdef Q_OS_WIN
                                                tr("Executable Files (*.exe);;All Files (*.*)")
#else
                                                tr("All Files (*)")
#endif
                                                );
    if (!path.isEmpty()) {
        m_externalEditorEdit->setText(QDir::toNativeSeparators(path));
    }
}

bool PreferencesDialog::eventFilter(QObject *watched, QEvent *event)
{
    // Handle left click on the external editor field (read-only QLineEdit acting as button)
    if (watched == m_externalEditorEdit && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            onBrowseExternalEditor();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
