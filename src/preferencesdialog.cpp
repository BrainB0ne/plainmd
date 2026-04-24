#include "preferencesdialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QSettings>
#include <QApplication>

PreferencesDialog::PreferencesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Preferences"));
    resize(400, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Editor group
    QGroupBox *editorGroup = new QGroupBox(tr("Editor"), this);
    QGridLayout *editorLayout = new QGridLayout(editorGroup);
    editorLayout->addWidget(new QLabel(tr("Font:"), this), 0, 0);
    m_fontCombo = new QFontComboBox(this);
    editorLayout->addWidget(m_fontCombo, 0, 1);
    editorLayout->addWidget(new QLabel(tr("Size:"), this), 1, 0);
    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(8, 72);
    editorLayout->addWidget(m_sizeSpin, 1, 1);
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
    m_fontCombo->setCurrentFont(QFont(settings.value("editor/fontFamily", "Segoe UI").toString()));
    m_sizeSpin->setValue(settings.value("editor/fontSize", 11).toInt());
    m_previewCheck->setChecked(settings.value("privacy/previewExternalImages", true).toBool());
}

void PreferencesDialog::saveSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QApplication::organizationName(), QApplication::applicationName());
    settings.setValue("editor/fontFamily", m_fontCombo->currentText());
    settings.setValue("editor/fontSize", m_sizeSpin->value());
    settings.setValue("privacy/previewExternalImages", m_previewCheck->isChecked());
}

QString PreferencesDialog::fontFamily() const
{
    return m_fontCombo->currentText();
}

int PreferencesDialog::fontSize() const
{
    return m_sizeSpin->value();
}

bool PreferencesDialog::previewExternalImages() const
{
    return m_previewCheck->isChecked();
}
