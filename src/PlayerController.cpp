#include "PlayerController.h"

#include <QMetaObject>
#include <cmath>
#include <algorithm>

PlayerController::PlayerController(QObject* parent) : QObject(parent) {
    m_sharedPos = std::make_shared<std::atomic<qint64>>(0);
    m_workerThread = new QThread(this);
    m_engine = new PlayerEngine(m_sharedPos);
    m_engine->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::finished, m_engine, &QObject::deleteLater);
    connect(m_engine, &PlayerEngine::playbackStopped, this, &PlayerController::onEngineStopped);
    m_workerThread->start();
    m_syncTimer = new QTimer(this);
    m_syncTimer->setInterval(16.6666667);
    connect(m_syncTimer, &QTimer::timeout, this, &PlayerController::onTimerTimeout);
}

PlayerController::~PlayerController() {
    stop();
    m_workerThread->quit();
    m_workerThread->wait();
}

void PlayerController::load(QSharedPointer<XPlayerProvider> provider) {
    stop();
    m_currentProvider = provider;
}

void PlayerController::play() {
    if (!m_currentProvider) return;
    if (m_state != Playing) {
        if (m_state == Stopped && m_currentProvider->atEnd()) {
            m_currentProvider->seek(0.0); 
            m_baseEngineUSecs = 0;
            m_sharedPos->store(0, std::memory_order_relaxed);
            emit timeChanged(0.0);
        }
        
        if (m_state == Paused) {
             QMetaObject::invokeMethod(m_engine, "resume", Qt::QueuedConnection);
        } else {
             QMetaObject::invokeMethod(m_engine, "start", Qt::QueuedConnection, 
                                       Q_ARG(QSharedPointer<XPlayerProvider>, m_currentProvider));
        }
        m_state = Playing;
        m_baseEngineUSecs = -1; 
        m_physicsTimer.start();
        m_syncTimer->start();
        emit stateChanged(m_state);
    }
}

void PlayerController::pause() {
    if (m_state == Playing) {
        QMetaObject::invokeMethod(m_engine, "pause", Qt::QueuedConnection);
        m_state = Paused;
        m_syncTimer->stop();
        emit stateChanged(m_state);
    }
}

void PlayerController::stop() {
    QMetaObject::invokeMethod(m_engine, "stop", Qt::QueuedConnection);
    m_state = Stopped;
    m_syncTimer->stop();
    m_baseEngineUSecs = -1;
    m_sharedPos->store(0, std::memory_order_relaxed);
    emit stateChanged(m_state);
}

void PlayerController::seek(double seconds) {
    if (!m_currentProvider) return;
    if (m_state == Stopped) {
        m_currentProvider->seek(seconds);
    } else {
        QMetaObject::invokeMethod(m_engine, "seek", Qt::QueuedConnection, Q_ARG(double, seconds));
    }
    m_baseEngineUSecs = static_cast<qint64>(seconds * 1000000.0);
    m_sharedPos->store(m_baseEngineUSecs, std::memory_order_relaxed);
    m_physicsTimer.restart();
    m_lastSeekTimer.restart(); 
    emit timeChanged(seconds);
}

void PlayerController::setUpdateRate(int fps) {
    if (fps < 15) fps = 15;
    if (fps > 300) fps = 300;
    int intervalMs = 1000 / fps;
    if (m_syncTimer) {
        m_syncTimer->setInterval(intervalMs);
    }
}

void PlayerController::onEngineStopped() {
    if (m_state == Playing) {
        stop();
        if (m_currentProvider && m_currentProvider->atEnd()) {
            seek(0.0);
        }
    }
}

void PlayerController::onTimerTimeout() {
    if (m_state == Playing) {
        qint64 currentEngineUSecs = m_sharedPos->load(std::memory_order_relaxed);
        if (std::abs(currentEngineUSecs - m_baseEngineUSecs) > 100000) {
            if (m_lastSeekTimer.isValid() && m_lastSeekTimer.elapsed() < 250) {
            } else {
                m_baseEngineUSecs = currentEngineUSecs;
                m_physicsTimer.restart();
            }
        } 
        else if (currentEngineUSecs > m_baseEngineUSecs) {
            m_baseEngineUSecs = currentEngineUSecs;
            m_physicsTimer.restart();
        }
        qint64 elapsedSinceUpdate = m_physicsTimer.nsecsElapsed() / 1000;
        if (elapsedSinceUpdate > 50000) elapsedSinceUpdate = 50000;
        qint64 smoothUSecs = m_baseEngineUSecs + elapsedSinceUpdate;
        double seconds = static_cast<double>(smoothUSecs) / 1000000.0;
        emit timeChanged(seconds);
    }
}

double PlayerController::getCurrentTime() const {
    if (m_sharedPos) {
        return static_cast<double>(m_sharedPos->load(std::memory_order_relaxed)) / 1000000.0;
    }
    return 0.0;
}