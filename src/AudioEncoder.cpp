#include "AudioEncoder.h"
#include <QDataStream>
#include <QIODevice>

QByteArray AudioEncoder::toWav(const QByteArray &pcm, int sampleRate, int bitsPerSample, int channels)
{
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    int blockAlign = channels * bitsPerSample / 8;
    int dataSize = pcm.size();
    int fileSize = 36 + dataSize;

    QByteArray wav;
    QDataStream out(&wav, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);

    // RIFF header
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(fileSize);
    out.writeRawData("WAVE", 4);

    // fmt subchunk
    out.writeRawData("fmt ", 4);
    out << static_cast<quint32>(16);           // subchunk size
    out << static_cast<quint16>(1);            // PCM format
    out << static_cast<quint16>(channels);
    out << static_cast<quint32>(sampleRate);
    out << static_cast<quint32>(byteRate);
    out << static_cast<quint16>(blockAlign);
    out << static_cast<quint16>(bitsPerSample);

    // data subchunk
    out.writeRawData("data", 4);
    out << static_cast<quint32>(dataSize);
    out.writeRawData(pcm.constData(), dataSize);

    return wav;
}
