#pragma once

#include "PlayerEngine.h"
#include "XPlayerProvider.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <atomic>
#include <memory>

class PlayerController : public QObject {
    Q_OBJECT

public:
    enum State {
        Stopped,
        Playing,
        Paused
    };
    Q_ENUM(State)
    explicit PlayerController(QObject* parent = nullptr);
    ~PlayerController();
    void load(QSharedPointer<XPlayerProvider> provider);
    void play();
    void pause();
    void stop();
    void seek(double seconds);
    void setUpdateRate(int fps);
    State getState() const { return m_state; }
    double getCurrentTime() const;

signals:
    void stateChanged(PlayerController::State state);
    void timeChanged(double seconds);

private slots:
    void onTimerTimeout();
    void onEngineStopped();

private:
    QThread* m_workerThread = nullptr;
    PlayerEngine* m_engine = nullptr;
    QSharedPointer<XPlayerProvider> m_currentProvider;
    State m_state = Stopped;
    QTimer* m_syncTimer = nullptr;
    qint64 m_baseEngineUSecs = -1;
    QElapsedTimer m_physicsTimer;
    std::shared_ptr<std::atomic<qint64>> m_sharedPos;
    QElapsedTimer m_lastSeekTimer;
};