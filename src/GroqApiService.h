#pragma once

#include "TranscriptionService.h"
#include <QNetworkAccessManager>

/// Groq Whisper API client — sends WAV as multipart/form-data POST.
class GroqApiService : public TranscriptionService {
    Q_OBJECT
public:
    explicit GroqApiService(const QString &apiKey, QObject *parent = nullptr);

    void transcribe(const QByteArray &wavData) override;
    void setApiKey(const QString &key);

private:
    QNetworkAccessManager m_nam;
    QString m_apiKey;
};
