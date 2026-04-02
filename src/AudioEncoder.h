#pragma once

#include <QByteArray>

/// Encodes raw PCM data into WAV format by prepending a 44-byte RIFF header.
/// No external library needed — pure QDataStream.
class AudioEncoder {
public:
    static QByteArray toWav(const QByteArray &pcm,
                            int sampleRate = 48000,
                            int bitsPerSample = 16,
                            int channels = 1);
};
