#pragma once

#include "XPlayerProvider.h"

#include <QIODevice>
#include <QSharedPointer>

class PlayerAudioSource : public QIODevice {
    Q_OBJECT

public:
    explicit PlayerAudioSource(QSharedPointer<XPlayerProvider> provider, QObject* parent = nullptr);
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

private:
    QSharedPointer<XPlayerProvider> m_provider;
};