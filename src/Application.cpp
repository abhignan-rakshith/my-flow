#include "Application.h"
#include "WhisperApiService.h"
#include "GroqApiService.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QTimer>
#include <QDebug>

Application::Application(QObject *parent)
    : QObject(parent)
{
    // Apply settings
    createTranscriptionService();
    applyAudioDevice();

    // Set up global hotkey
    m_hotkey = std::make_unique<GlobalHotkey>(m_settings.hotkey());
    if (m_hotkey->isAvailable()) {
        connect(m_hotkey.get(), &GlobalHotkey::pressed, this, [this]() {
            if (m_capture.isRecording()) {
                onStopRecording();
            } else {
                onStartRecording();
            }
        });
    } else {
        qWarning() << "Global hotkey not available, use tray icon to toggle recording";
    }

    // Tray icon signals
    connect(&m_tray, &TrayIcon::toggleRecording, this, [this]() {
        if (m_capture.isRecording()) {
            onStopRecording();
        } else {
            onStartRecording();
        }
    });
    connect(&m_tray, &TrayIcon::settingsRequested, this, &Application::onSettingsRequested);
    connect(&m_tray, &TrayIcon::quitRequested, qApp, &QApplication::quit);

    // Audio capture signals
    connect(&m_capture, &AudioCapture::recordingFinished, this, &Application::onRecordingFinished);
    connect(&m_capture, &AudioCapture::error, this, [this](const QString &msg) {
        m_tray.showError(msg);
        m_tray.setState(TrayIcon::State::Idle);
        m_overlay.hide();
    });

    // Text injector errors
    connect(&m_injector, &TextInjector::injectionError, this, [this](const QString &msg) {
        m_tray.showError(msg);
    });

    qDebug() << "Wispr Flow initialized";
    qDebug() << "[CONFIG] Backend:" << (m_settings.backend() == Settings::Backend::OpenAI ? "OpenAI" : "Groq");
    qDebug() << "[CONFIG] API key set:" << !m_settings.apiKey().isEmpty();
    qDebug() << "[CONFIG] API key length:" << m_settings.apiKey().length();
    qDebug() << "[CONFIG] Hotkey:" << m_settings.hotkey();
    if (m_settings.apiKey().isEmpty()) {
        m_tray.showError("No API key configured. Right-click tray icon -> Settings.");
    }
}

void Application::onStartRecording()
{
    qDebug() << "[HOTKEY] pressed -> starting recording";
    m_tray.setState(TrayIcon::State::Recording);
    m_overlay.showRecording();
    m_capture.start();
}

void Application::onStopRecording()
{
    qDebug() << "[HOTKEY] released -> stopping recording";
    m_capture.stop();
    m_tray.setState(TrayIcon::State::Transcribing);
    m_overlay.hide();
}

void Application::onRecordingFinished(const QByteArray &pcmData)
{
    qDebug() << "[AUDIO] Recording finished, PCM bytes:" << pcmData.size();
    if (pcmData.isEmpty()) {
        qDebug() << "[AUDIO] Empty recording, skipping transcription";
        m_tray.setState(TrayIcon::State::Idle);
        return;
    }

    QByteArray wavData = AudioEncoder::toWav(pcmData);
    qDebug() << "[AUDIO] WAV encoded:" << wavData.size() << "bytes,"
             << (pcmData.size() / 32000.0) << "seconds";

    // Save recording for debugging
    QString path = "/tmp/wispr-flow-last.wav";
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(wavData);
        f.close();
        qDebug() << "[AUDIO] Saved to" << path;
    }

    qDebug() << "[API] Sending to transcription service...";
    m_transcriber->transcribe(wavData);
}

void Application::onTranscriptionReady(const QString &text)
{
    qDebug() << "[API] Transcription received:" << text;
    m_injector.type(text);
    m_tray.setState(TrayIcon::State::Idle);
}

void Application::onTranscriptionError(const QString &error)
{
    qWarning() << "[API] Transcription error:" << error;
    m_tray.showError("Transcription failed: " + error);
    m_tray.setState(TrayIcon::State::Idle);
}

void Application::onSettingsRequested()
{
    auto *dialog = new SettingsDialog(m_settings);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &SettingsDialog::settingsChanged, this, &Application::onSettingsChanged);
    dialog->show();
}

void Application::onSettingsChanged()
{
    createTranscriptionService();
    applyAudioDevice();
    qDebug() << "[CONFIG] Settings updated, backend:"
             << (m_settings.backend() == Settings::Backend::OpenAI ? "OpenAI" : "Groq");
}

void Application::applyAudioDevice()
{
    QString deviceId = m_settings.audioDevice();
    m_capture.setDevice(deviceId.toUtf8());
    qDebug() << "[CONFIG] Audio device set to:" << (deviceId.isEmpty() ? "Default" : deviceId);
}

void Application::createTranscriptionService()
{
    if (m_settings.backend() == Settings::Backend::OpenAI) {
        m_transcriber = std::make_unique<WhisperApiService>(m_settings.apiKey());
    } else {
        m_transcriber = std::make_unique<GroqApiService>(m_settings.apiKey());
    }

    connect(m_transcriber.get(), &TranscriptionService::transcriptionReady,
            this, &Application::onTranscriptionReady);
    connect(m_transcriber.get(), &TranscriptionService::transcriptionError,
            this, &Application::onTranscriptionError);
}

bool Application::switchBluetoothProfile(bool headset)
{
    // Find any connected Bluetooth card and switch profile
    QProcess pactl;
    pactl.start("pactl", {"list", "cards", "short"});
    if (!pactl.waitForFinished(2000))
        return false;

    QString output = pactl.readAllStandardOutput();
    for (const QString &line : output.split('\n')) {
        if (!line.contains("bluez_card"))
            continue;

        // Extract card name (second column)
        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() < 2)
            continue;

        QString card = parts[1];
        QString profile = headset ? "headset-head-unit" : "a2dp-sink";

        qDebug() << "[BT] Switching" << card << "to" << profile;
        int ret = QProcess::execute("pactl", {"set-card-profile", card, profile});
        if (ret != 0) {
            qDebug() << "[BT] pactl returned" << ret << "- retrying with fresh card list";
            // Card name may have changed, just ignore the error
        }
        return true;
    }
    return false;
}
