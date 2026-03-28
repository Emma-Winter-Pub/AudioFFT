#pragma once

#include "IPlayerProvider.h"
#include "PlayerAudioSource.h"

#include <QObject>
#include <QAudioSink>
#include <QMutex>
#include <QAudioDevice>
#include <QSharedPointer>

class PlayerEngine : public QObject {
    Q_OBJECT

public:
    explicit PlayerEngine(QObject* parent = nullptr);
    ~PlayerEngine();
    Q_INVOKABLE qint64 getCorrectedPositionUSecs(); 

public slots:
    void start(QSharedPointer<IPlayerProvider> provider);
    void stop();
    void pause();
    void resume();
    void seek(double seconds);

private slots:
    void onDefaultDeviceChanged(const QAudioDevice &device);

signals:
    void playbackStopped();

private:
    QAudioSink* m_audioSink = nullptr;
    PlayerAudioSource* m_audioSource = nullptr;
    QSharedPointer<IPlayerProvider> m_currentProvider;
    QAudioFormat m_currentFormat; 
    double m_baseOffsetSec = 0.0;
};