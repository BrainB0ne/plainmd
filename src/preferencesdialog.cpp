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
    resize(400, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Editor group
    QGroupBox *editorGroup = new QGroupBox(tr("Editor"), this);
    QGridLayout *editorLayout = new QGridLayout(editorGroup);
    m_fontButton = new QPushButton(tr("Choose Font..."), this);
    connect(m_fontButton, &QPushButton::clicked, this, &PreferencesDialog::onChooseFont);
    editorLayout->addWidget(m_fontButton, 0, 0);
    m_fontLabel = new QLabel(this);
    m_fontLabel->setWordWrap(true);
    editorLayout->addWidget(m_fontLabel, 1, 0);
    mainLayout->addWidget(editorGroup);

    // Privacy group
    QGroupBox *privacyGroup = new QGroupBox(tr("Privacy"), this);
    QVBoxLayout *privacyLayout = new QVBoxLayout(privacyGroup);
    m_previewCheck = new QCheckBox(tr("Preview external images"), this);
    privacyLayout->addWidget(m_previewCheck);
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
    QString family = settings.value("editor/fontFamily", "Segoe UI").toString();
    int size = settings.value("editor/fontSize", 11).toInt();
    m_currentFont = QFont(family);
    m_currentFont.setPointSize(size);
    m_fontLabel->setText(QStringLiteral("%1, %2 pt").arg(m_currentFont.family()).arg(m_currentFont.pointSize()));
    m_previewCheck->setChecked(settings.value("privacy/previewExternalImages", true).toBool());
}

void PreferencesDialog::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("editor/fontFamily", m_currentFont.family());
    settings.setValue("editor/fontSize", m_currentFont.pointSize());
    settings.setValue("privacy/previewExternalImages", m_previewCheck->isChecked());
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
    return m_previewCheck->isChecked();
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
