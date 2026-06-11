#include "PlayerAudioSource.h"

PlayerAudioSource::PlayerAudioSource(QSharedPointer<IPlayerProvider> provider, QObject* parent)
    : QIODevice(parent), m_provider(provider)
{
    open(QIODevice::ReadOnly);
}

qint64 PlayerAudioSource::readData(char* data, qint64 maxlen) {
    if (!m_provider) return 0;
    qint64 maxFloats = maxlen / sizeof(float);
    qint64 floatsRead = m_provider->readSamples(reinterpret_cast<float*>(data), maxFloats);
    if (floatsRead == 0 && m_provider->atEnd()) {}
    return floatsRead * sizeof(float);
}

qint64 PlayerAudioSource::writeData(const char* data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

bool PlayerAudioSource::isSequential() const {
    return true;
}

qint64 PlayerAudioSource::bytesAvailable() const {
    if (m_provider && !m_provider->atEnd()) {
        return 4096;
    }
    return 0;
}