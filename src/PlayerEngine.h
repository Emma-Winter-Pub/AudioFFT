#pragma once

#include "IPlayerProvider.h"
#include "PlayerAudioSource.h"

#include <QObject>
#include <QAudioSink>
#include <QMutex>
#include <QAudioDevice>
#include <QSharedPointer>
#include <QTimer>
#include <atomic>
#include <memory>

class PlayerEngine : public QObject {
    Q_OBJECT

public:
    explicit PlayerEngine(std::shared_ptr<std::atomic<qint64>> sharedPos, QObject* parent = nullptr);
    ~PlayerEngine();

public slots:
    void start(QSharedPointer<IPlayerProvider> provider);
    void stop();
    void pause();
    void resume();
    void seek(double seconds);

private slots:
    void onDefaultDeviceChanged(const QAudioDevice &device);
    void updatePositionAtomic();

signals:
    void playbackStopped();

private:
    qint64 getCorrectedPositionUSecs();
    QAudioSink* m_audioSink = nullptr;
    PlayerAudioSource* m_audioSource = nullptr;
    QSharedPointer<IPlayerProvider> m_currentProvider;
    QAudioFormat m_currentFormat; 
    double m_baseOffsetSec = 0.0;
    std::shared_ptr<std::atomic<qint64>> m_sharedPos;
    QTimer* m_posTimer = nullptr;
};