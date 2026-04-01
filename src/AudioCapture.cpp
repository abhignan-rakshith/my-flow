#include "AudioCapture.h"
#include <QAudioFormat>
#include <QDebug>

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
{
}

bool AudioCapture::isRecording() const
{
    return m_recording;
}

void AudioCapture::start()
{
    if (m_recording)
        return;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    auto device = QMediaDevices::defaultAudioInput();
    if (!device.isFormatSupported(format)) {
        emit error("Default audio device does not support 16kHz/16-bit/mono");
        return;
    }

    m_data.clear();
    m_buffer.setBuffer(&m_data);
    m_buffer.open(QIODevice::WriteOnly);

    m_source = std::make_unique<QAudioSource>(device, format, this);
    m_source->start(&m_buffer);
    m_recording = true;

    qDebug() << "Recording started";
}

void AudioCapture::stop()
{
    if (!m_recording)
        return;

    m_source->stop();
    m_buffer.close();
    m_source.reset();
    m_recording = false;

    qDebug() << "Recording stopped," << m_data.size() << "bytes captured";

    emit recordingFinished(m_data);
}
