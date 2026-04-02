#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <memory>
#include "Settings.h"
#include "AudioCapture.h"
#include "AudioEncoder.h"
#include "TranscriptionService.h"
#include "GlobalHotkey.h"
#include "TextInjector.h"
#include "TrayIcon.h"
#include "OverlayWidget.h"

/// Central orchestrator — owns all components and wires them via signals/slots.
/// Flow: hotkey → record → encode → transcribe → LLM cleanup → type into focused window.
class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);

private slots:
    void onStartRecording();
    void onStopRecording();
    void onRecordingFinished(const QByteArray &pcmData);
    void onTranscriptionReady(const QString &text);
    void onTranscriptionError(const QString &error);
    void onSettingsRequested();
    void onSettingsChanged();

private:
    void createTranscriptionService();
    void applyAudioDevice();
    void postProcess(const QString &rawText);

    QNetworkAccessManager m_nam;

    Settings m_settings;
    AudioCapture m_capture;
    TextInjector m_injector;
    TrayIcon m_tray;
    OverlayWidget m_overlay;
    std::unique_ptr<GlobalHotkey> m_hotkey;
    std::unique_ptr<TranscriptionService> m_transcriber;
};
