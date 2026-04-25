#include "preferencesdialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSettings>
#include <QApplication>
#include <QFontDialog>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(400, 260);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Editor group
    QGroupBox *editorGroup = new QGroupBox(tr("Editor"), this);
    QGridLayout *editorLayout = new QGridLayout(editorGroup);

    editorLayout->addWidget(new QLabel(tr("Font:"), this), 0, 0);
    m_fontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_fontButton, &QPushButton::clicked, this, &PreferencesDialog::onChooseFont);
    editorLayout->addWidget(m_fontButton, 0, 1);
    m_fontLabel = new QLabel(this);
    m_fontLabel->setWordWrap(true);
    editorLayout->addWidget(m_fontLabel, 1, 0, 1, 2);

    editorLayout->addWidget(new QLabel(tr("Code Block Font:"), this), 2, 0);
    m_codeBlockFontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_codeBlockFontButton, &QPushButton::clicked, this, &PreferencesDialog::onChooseCodeBlockFont);
    editorLayout->addWidget(m_codeBlockFontButton, 2, 1);
    m_codeBlockFontLabel = new QLabel(this);
    m_codeBlockFontLabel->setWordWrap(true);
    editorLayout->addWidget(m_codeBlockFontLabel, 3, 0, 1, 2);

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
