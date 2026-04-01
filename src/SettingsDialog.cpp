#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QPushButton>

SettingsDialog::SettingsDialog(Settings &settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle("Wispr Flow Settings");
    setMinimumWidth(400);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    // Backend selector
    m_backendCombo = new QComboBox;
    m_backendCombo->addItem("Groq (Fast)", static_cast<int>(Settings::Backend::Groq));
    m_backendCombo->addItem("OpenAI Whisper", static_cast<int>(Settings::Backend::OpenAI));
    m_backendCombo->setCurrentIndex(
        m_settings.backend() == Settings::Backend::OpenAI ? 1 : 0);
    form->addRow("Backend:", m_backendCombo);

    // API Key
    m_apiKeyEdit = new QLineEdit;
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setText(m_settings.apiKey());
    m_apiKeyEdit->setPlaceholderText("Enter your API key...");

    auto *showKeyBtn = new QPushButton("Show");
    showKeyBtn->setCheckable(true);
    connect(showKeyBtn, &QPushButton::toggled, this, [this, showKeyBtn](bool checked) {
        m_apiKeyEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        showKeyBtn->setText(checked ? "Hide" : "Show");
    });

    auto *keyLayout = new QHBoxLayout;
    keyLayout->addWidget(m_apiKeyEdit);
    keyLayout->addWidget(showKeyBtn);
    form->addRow("API Key:", keyLayout);

    // Audio device
    m_audioDeviceCombo = new QComboBox;
    m_audioDeviceCombo->addItem("Default");
    for (const auto &dev : QMediaDevices::audioInputs()) {
        m_audioDeviceCombo->addItem(dev.description(), dev.id());
    }
    QString savedDevice = m_settings.audioDevice();
    if (!savedDevice.isEmpty()) {
        int idx = m_audioDeviceCombo->findData(savedDevice.toUtf8());
        if (idx >= 0) m_audioDeviceCombo->setCurrentIndex(idx);
    }
    form->addRow("Audio Device:", m_audioDeviceCombo);

    layout->addLayout(form);

    // Buttons
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::save()
{
    auto backend = static_cast<Settings::Backend>(
        m_backendCombo->currentData().toInt());
    m_settings.setBackend(backend);
    m_settings.setApiKey(m_apiKeyEdit->text().trimmed());

    QByteArray deviceId = m_audioDeviceCombo->currentData().toByteArray();
    m_settings.setAudioDevice(QString::fromUtf8(deviceId));

    emit settingsChanged();
    accept();
}
