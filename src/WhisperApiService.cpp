#include "WhisperApiService.h"
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

WhisperApiService::WhisperApiService(const QString &apiKey, QObject *parent)
    : TranscriptionService(parent)
    , m_apiKey(apiKey)
{
}

void WhisperApiService::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void WhisperApiService::transcribe(const QByteArray &wavData)
{
    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // file part
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"audio.wav\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    filePart.setBody(wavData);
    multiPart->append(filePart);

    // model part
    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"model\""));
    modelPart.setBody("whisper-1");
    multiPart->append(modelPart);

    QNetworkRequest request(QUrl("https://api.openai.com/v1/audio/transcriptions"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    request.setTransferTimeout(30000);

    auto *reply = m_nam.post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit transcriptionError(reply->errorString());
            return;
        }

        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        auto json = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit transcriptionError("Invalid JSON response: " + parseError.errorString());
            return;
        }

        if (json.object().contains("error")) {
            QString msg = json.object()["error"].toObject()["message"].toString();
            emit transcriptionError("API error: " + msg);
            return;
        }

        QString text = json.object().value("text").toString();
        if (text.isEmpty()) {
            emit transcriptionError("Empty transcription response");
            return;
        }

        emit transcriptionReady(text);
    });
}
