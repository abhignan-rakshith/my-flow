#include "GroqApiService.h"
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

GroqApiService::GroqApiService(const QString &apiKey, QObject *parent)
    : TranscriptionService(parent)
    , m_apiKey(apiKey)
{
}

void GroqApiService::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void GroqApiService::transcribe(const QByteArray &wavData)
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
    modelPart.setBody("whisper-large-v3-turbo");
    multiPart->append(modelPart);

    QNetworkRequest request(QUrl("https://api.groq.com/openai/v1/audio/transcriptions"));
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    auto *reply = m_nam.post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit transcriptionError(reply->errorString());
            return;
        }

        auto json = QJsonDocument::fromJson(reply->readAll());
        QString text = json.object().value("text").toString();
        if (text.isEmpty()) {
            emit transcriptionError("Empty transcription response");
            return;
        }

        emit transcriptionReady(text);
    });
}
