#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

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
