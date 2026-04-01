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
    qDebug() << "[AUDIO] Using device:" << device.description() << "id:" << device.id();

    if (!device.isFormatSupported(format)) {
        emit error("Default audio device does not support 16kHz/16-bit/mono");
        return;
    }

    m_data.clear();

    m_source = std::make_unique<QAudioSource>(device, format, this);
    m_source->setBufferSize(32000); // 1 second buffer

    connect(m_source.get(), &QAudioSource::stateChanged,
            this, &AudioCapture::onStateChanged);

    // Use pull mode: QAudioSource writes to a QIODevice we read from
    m_ioDevice = m_source->start();
    if (!m_ioDevice) {
        emit error("Failed to start audio capture");
        return;
    }

    connect(m_ioDevice, &QIODevice::readyRead, this, &AudioCapture::onReadyRead);

    m_recording = true;
    qDebug() << "[AUDIO] Recording started, state:" << m_source->state();
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
    qDebug() << "[AUDIO] State changed:" << state;
    if (state == QAudio::StoppedState && m_source) {
        if (m_source->error() != QAudio::NoError) {
            qWarning() << "[AUDIO] Error:" << m_source->error();
            emit error("Audio capture error: " + QString::number(m_source->error()));
        }
    }
}

void AudioCapture::stop()
{
    if (!m_recording)
        return;

    // Read any remaining data
    if (m_ioDevice) {
        QByteArray remaining = m_ioDevice->readAll();
        if (!remaining.isEmpty())
            m_data.append(remaining);
    }

    m_source->stop();
    m_ioDevice = nullptr;
    m_source.reset();
    m_recording = false;

    qDebug() << "[AUDIO] Recording stopped," << m_data.size() << "bytes captured";

    emit recordingFinished(m_data);
}
