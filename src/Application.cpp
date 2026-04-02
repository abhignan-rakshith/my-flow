#include "Application.h"
#include "WhisperApiService.h"
#include "GroqApiService.h"
#include "SettingsDialog.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>

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
    m_transcriber->transcribe(wavData);
}

void Application::onTranscriptionReady(const QString &text)
{
    // Post-process with LLM for cleanup
    postProcess(text);
}

void Application::postProcess(const QString &rawText)
{
    QJsonObject msg;
    msg["role"] = "user";
    msg["content"] = rawText;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are a dictation assistant. Clean up the following speech-to-text transcription. "
                           "Fix grammar, punctuation, and capitalization. Remove filler words (um, uh, like). "
                           "When the speaker corrects themselves (e.g. 'wrist, fist I mean'), use the CORRECTION, not the original word. "
                           "Do NOT change the meaning or add new content. Output ONLY the cleaned text, nothing else.";

    QJsonObject body;
    body["model"] = "llama-3.1-8b-instant";
    body["messages"] = QJsonArray{systemMsg, msg};
    body["temperature"] = 0.1;
    body["max_tokens"] = 2048;

    QNetworkRequest request(QUrl("https://api.groq.com/openai/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_settings.apiKey()).toUtf8());
    request.setTransferTimeout(15000);

    auto *reply = m_nam.post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply, rawText]() {
        reply->deleteLater();

        // If post-processing fails, fall back to raw text
        if (reply->error() != QNetworkReply::NoError) {
            m_injector.type(rawText);
            m_tray.setState(TrayIcon::State::Idle);
            return;
        }

        QJsonParseError parseError;
        auto json = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            m_injector.type(rawText);
            m_tray.setState(TrayIcon::State::Idle);
            return;
        }

        QString cleaned = json.object()["choices"].toArray()
                              .at(0).toObject()["message"].toObject()
                              ["content"].toString().trimmed();

        if (cleaned.isEmpty()) {
            cleaned = rawText;
        }

        m_injector.type(cleaned);
        m_tray.setState(TrayIcon::State::Idle);
    });
}

void Application::onTranscriptionError(const QString &error)
{
    qWarning() << "TRANSCRIPTION ERROR:" << error;
    m_tray.showError("Transcription failed: " + error);
    m_tray.setState(TrayIcon::State::Idle);
}

void Application::onSettingsRequested()
{
    auto *dialog = new SettingsDialog(m_settings);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    connect(dialog, &SettingsDialog::settingsChanged, this, &Application::onSettingsChanged);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
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
