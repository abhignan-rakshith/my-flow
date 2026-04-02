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

    // 48kHz/16-bit/mono — high enough quality for Whisper while keeping file size
    // reasonable. Mono is fine since we only need voice from one source.
    QAudioFormat format;
    format.setSampleRate(48000);
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
        emit error("Audio device does not support 48kHz/16-bit/mono");
        return;
    }

    m_data.clear();

    m_source = std::make_unique<QAudioSource>(device, format, this);
    m_source->setBufferSize(96000);
    m_source->setVolume(1.0f);

    connect(m_source.get(), &QAudioSource::stateChanged,
            this, &AudioCapture::onStateChanged);

    // Pull mode: start() returns a QIODevice* we read from on readyRead.
    // Push mode (start(&QBuffer)) was unreliable — QBuffer didn't grow consistently.
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

    // Audio processing pipeline: high-pass → noise gate → normalize
    if (m_data.size() >= 2) {
        auto *samples = reinterpret_cast<qint16 *>(m_data.data());
        int count = m_data.size() / sizeof(qint16);

        // 1) High-pass filter at 200Hz (2nd order Butterworth at 48kHz)
        //    Removes hum and low-frequency rumble below voice range
        {
            const double a1 = -1.9741953, a2 = 0.9745483,
                         b0 = 0.9872448,  b1 = -1.9744896, b2 = 0.9872448;
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            for (int i = 0; i < count; ++i) {
                double x0 = samples[i];
                double y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = x0;
                y2 = y1; y1 = y0;
                int out = static_cast<int>(y0);
                if (out > 32767) out = 32767;
                if (out < -32768) out = -32768;
                samples[i] = static_cast<qint16>(out);
            }
        }

        // 2) Noise gate — suppress samples below a threshold to kill mic self-noise
        //    Compute RMS over sliding window; attenuate quiet sections
        {
            const int windowSize = 480; // 10ms at 48kHz
            const double gateThreshold = 150.0; // RMS below this = noise
            const float attenuation = 0.05f; // how much to keep when gated

            for (int i = 0; i < count; i += windowSize) {
                int end = (i + windowSize < count) ? i + windowSize : count;
                double sum = 0;
                for (int j = i; j < end; ++j)
                    sum += static_cast<double>(samples[j]) * samples[j];
                double rms = std::sqrt(sum / (end - i));

                if (rms < gateThreshold) {
                    for (int j = i; j < end; ++j)
                        samples[j] = static_cast<qint16>(samples[j] * attenuation);
                }
            }
        }

        // 3) Normalize to -6dB (target ~16000) then soft-limit peaks
        {
            const float targetPeak = 16000.0f; // -6dB headroom
            const float softLimit  = 24000.0f; // knee where compression starts
            const float hardCeil   = 32000.0f; // absolute ceiling

            qint16 peak = 0;
            for (int i = 0; i < count; ++i) {
                qint16 abs = samples[i] < 0 ? -samples[i] : samples[i];
                if (abs > peak) peak = abs;
            }
            if (peak > 0 && peak < static_cast<qint16>(targetPeak)) {
                float gain = targetPeak / peak;
                for (int i = 0; i < count; ++i)
                    samples[i] = static_cast<qint16>(samples[i] * gain);
            }

            // Soft limiter — tanh-based compression above the knee
            for (int i = 0; i < count; ++i) {
                float s = samples[i];
                if (s > softLimit) {
                    s = softLimit + (hardCeil - softLimit) * std::tanh((s - softLimit) / (hardCeil - softLimit));
                } else if (s < -softLimit) {
                    s = -(softLimit + (hardCeil - softLimit) * std::tanh((-s - softLimit) / (hardCeil - softLimit)));
                }
                samples[i] = static_cast<qint16>(s);
            }
        }
    }

    emit recordingFinished(m_data);
}
