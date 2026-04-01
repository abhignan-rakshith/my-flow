#pragma once

#include <QObject>
#include <QByteArray>
#include <QAudioSource>
#include <QMediaDevices>
#include <QTimer>
#include <QIODevice>
#include <memory>

class AudioCapture : public QObject {
    Q_OBJECT
public:
    explicit AudioCapture(QObject *parent = nullptr);

    bool isRecording() const;
    void setDevice(const QByteArray &deviceId);

public slots:
    void start();
    void stop();

signals:
    void recordingFinished(const QByteArray &rawPcm);
    void error(const QString &message);

private slots:
    void onReadyRead();
    void onStateChanged(QAudio::State state);

private:
    std::unique_ptr<QAudioSource> m_source;
    QIODevice *m_ioDevice = nullptr;
    QByteArray m_data;
    QByteArray m_deviceId;
    bool m_recording = false;
};
