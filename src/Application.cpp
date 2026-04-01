#include "Application.h"
#include "WhisperApiService.h"
#include "GroqApiService.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QFile>
#include <QDateTime>

Application::Application(QObject *parent)
    : QObject(parent)
{
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

    if (m_settings.apiKey().isEmpty()) {
        m_tray.showError("No API key configured. Right-click tray icon -> Settings.");
    }
}

void Application::onStartRecording()
{
    m_tray.setState(TrayIcon::State::Recording);
    m_overlay.showRecording();
    m_capture.start();
}

void Application::onStopRecording()
{
    m_capture.stop();
    m_tray.setState(TrayIcon::State::Transcribing);
    m_overlay.hide();
}

void Application::onRecordingFinished(const QByteArray &pcmData)
{
    if (pcmData.isEmpty()) {
        m_tray.setState(TrayIcon::State::Idle);
        return;
    }

    QByteArray wavData = AudioEncoder::toWav(pcmData);

    // Save recording
    QFile wav("/tmp/wispr-flow-last.wav");
    if (wav.open(QIODevice::WriteOnly)) {
        wav.write(wavData);
        wav.close();
    }

    m_transcriber->transcribe(wavData);
}

void Application::onTranscriptionReady(const QString &text)
{
    // Append transcript to log
    QFile log("/tmp/wispr-flow-transcripts.log");
    if (log.open(QIODevice::Append | QIODevice::Text)) {
        log.write(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ").toUtf8());
        log.write(text.toUtf8());
        log.write("\n");
        log.close();
    }

    m_injector.type(text);
    m_tray.setState(TrayIcon::State::Idle);
}

void Application::onTranscriptionError(const QString &error)
{
    qWarning() << "Transcription error:" << error;
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
}

void Application::applyAudioDevice()
{
    QString deviceId = m_settings.audioDevice();
    m_capture.setDevice(deviceId.toUtf8());
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
