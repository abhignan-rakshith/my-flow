#pragma once

#include <QByteArray>

class AudioEncoder {
public:
    static QByteArray toWav(const QByteArray &pcm,
                            int sampleRate = 16000,
                            int bitsPerSample = 16,
                            int channels = 1);
};
