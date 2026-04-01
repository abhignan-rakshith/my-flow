#include "AudioCapture.h"
#include <QAudioFormat>

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
{
}

bool AudioCapture::isRecording() const
{
    return m_recording;
}

void AudioCapture::setDevice(const QByteArray &deviceId)
{
    m_deviceId = deviceId;
}

void AudioCapture::start()
{
    if (m_recording)
        return;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    // Find selected device or use default
    QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (!m_deviceId.isEmpty()) {
        for (const auto &dev : QMediaDevices::audioInputs()) {
            if (dev.id() == m_deviceId) {
                device = dev;
                break;
            }
        }
    }

    if (!device.isFormatSupported(format)) {
        emit error("Audio device does not support 16kHz/16-bit/mono");
        return;
    }

    m_data.clear();

    m_source = std::make_unique<QAudioSource>(device, format, this);
    m_source->setBufferSize(32000);

    connect(m_source.get(), &QAudioSource::stateChanged,
            this, &AudioCapture::onStateChanged);

    m_ioDevice = m_source->start();
    if (!m_ioDevice) {
        emit error("Failed to start audio capture");
        return;
    }

    connect(m_ioDevice, &QIODevice::readyRead, this, &AudioCapture::onReadyRead);
    m_recording = true;
}

void AudioCapture::onReadyRead()
{
    if (!m_ioDevice)
        return;

    QByteArray chunk = m_ioDevice->readAll();
    if (!chunk.isEmpty()) {
        m_data.append(chunk);
    }
}

void AudioCapture::onStateChanged(QAudio::State state)
{
    if (state == QAudio::StoppedState && m_source) {
        if (m_source->error() != QAudio::NoError) {
            emit error("Audio capture error: " + QString::number(m_source->error()));
        }
    }
}

void AudioCapture::stop()
{
    if (!m_recording)
        return;

    if (m_ioDevice) {
        QByteArray remaining = m_ioDevice->readAll();
        if (!remaining.isEmpty())
            m_data.append(remaining);
    }

    m_source->stop();
    m_ioDevice = nullptr;
    m_source.reset();
    m_recording = false;

    emit recordingFinished(m_data);
}
