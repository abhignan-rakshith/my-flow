#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

/// Abstract interface for speech-to-text backends (Groq, OpenAI).
/// Accepts WAV data, emits transcribed text or error asynchronously.
class TranscriptionService : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    virtual ~TranscriptionService() = default;

    virtual void transcribe(const QByteArray &wavData) = 0;

signals:
    void transcriptionReady(const QString &text);
    void transcriptionError(const QString &error);
};
