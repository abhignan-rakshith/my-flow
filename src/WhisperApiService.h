#pragma once

#include "TranscriptionService.h"
#include <QNetworkAccessManager>

/// OpenAI Whisper API client — same multipart/form-data format, different endpoint.
class WhisperApiService : public TranscriptionService {
    Q_OBJECT
public:
    explicit WhisperApiService(const QString &apiKey, QObject *parent = nullptr);

    void transcribe(const QByteArray &wavData) override;
    void setApiKey(const QString &key);

private:
    QNetworkAccessManager m_nam;
    QString m_apiKey;
};
