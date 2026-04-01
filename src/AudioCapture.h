#pragma once

#include <QObject>
#include <QByteArray>
#include <QBuffer>
#include <QAudioSource>
#include <QMediaDevices>
#include <memory>

class AudioCapture : public QObject {
    Q_OBJECT
public:
    explicit AudioCapture(QObject *parent = nullptr);

    bool isRecording() const;

public slots:
    void start();
    void stop();

signals:
    void recordingFinished(const QByteArray &rawPcm);
    void error(const QString &message);

private:
    std::unique_ptr<QAudioSource> m_source;
    QBuffer m_buffer;
    QByteArray m_data;
    bool m_recording = false;
};
