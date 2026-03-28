#include "PlayerEngine.h"
#include "AppConfig.h"

#include <QMediaDevices>
#include <QAudioDevice>

PlayerEngine::PlayerEngine(QObject* parent) : QObject(parent) {
    QMediaDevices* deviceMonitor = new QMediaDevices(this);
    connect(deviceMonitor, &QMediaDevices::audioOutputsChanged, 
            this, [this]() {
                onDefaultDeviceChanged(QMediaDevices::defaultAudioOutput());
            });
}

PlayerEngine::~PlayerEngine() {stop();}

qint64 PlayerEngine::getCorrectedPositionUSecs() {
    if (!m_currentProvider) {
        return static_cast<qint64>(m_baseOffsetSec * 1000000.0);
    }
    if (!m_audioSink || m_audioSink->state() == QAudio::StoppedState) {
        return static_cast<qint64>(m_currentProvider->currentTime() * 1000000.0);
    }
    double decodedHeadTime = m_currentProvider->getLastDecodedTime();
    double internalDelay = m_currentProvider->getInternalBufferDuration();
    qint64 sinkBufferBytes = m_audioSink->bufferSize();
    int sampleRate = m_currentFormat.sampleRate();
    if (sampleRate <= 0) sampleRate = 44100;
    int channels = m_currentFormat.channelCount();
    double bytesPerSec = static_cast<double>(sampleRate * channels * sizeof(float));
    double sinkDelay = 0.0;
    if (bytesPerSec > 0) {
        sinkDelay = static_cast<double>(sinkBufferBytes) / bytesPerSec;
    }
    double realTime = decodedHeadTime - internalDelay - sinkDelay;
    double compensation = static_cast<double>(CoreConfig::PLAYER_LATENCY_COMPENSATION_US) / 1000000.0;
    realTime += compensation;
    if (realTime < 0) realTime = 0;
    return static_cast<qint64>(realTime * 1000000.0);
}

void PlayerEngine::start(QSharedPointer<IPlayerProvider> provider) {
    stop(); 
    m_currentProvider = provider;
    if (!m_currentProvider) return;
    m_baseOffsetSec = m_currentProvider->currentTime(); 
    int nativeRate = m_currentProvider->getNativeSampleRate();
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    QAudioFormat preferredFormat;
    preferredFormat.setSampleRate(nativeRate);
    preferredFormat.setChannelCount(2);
    preferredFormat.setSampleFormat(QAudioFormat::Float);
    if (device.isFormatSupported(preferredFormat)) {
        m_currentFormat = preferredFormat;
    } else {
        QAudioFormat fallbackFormat = preferredFormat;
        fallbackFormat.setSampleRate(44100);
        if (device.isFormatSupported(fallbackFormat)) {
            m_currentFormat = fallbackFormat;
        } else {
            fallbackFormat.setSampleRate(48000);
            if (device.isFormatSupported(fallbackFormat)) {
                m_currentFormat = fallbackFormat;
            } else {
                m_currentFormat = device.preferredFormat();
                m_currentFormat.setChannelCount(2);
                m_currentFormat.setSampleFormat(QAudioFormat::Float);
            }
        }
    }
    m_currentProvider->configureResampler(m_currentFormat.sampleRate());
    m_audioSink = new QAudioSink(device, m_currentFormat, this);
    int sampleRate = m_currentFormat.sampleRate();
    m_audioSink->setBufferSize(sampleRate * 2 * sizeof(float) * 0.1); 
    m_audioSource = new PlayerAudioSource(m_currentProvider, this);
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State state){
        if (state == QAudio::IdleState && m_audioSink->error() == QAudio::NoError) {
            m_audioSink->stop(); 
            emit playbackStopped();
        }
    });
    m_audioSink->start(m_audioSource);
}

void PlayerEngine::stop() {
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    m_currentProvider = nullptr;
    m_baseOffsetSec = 0.0;
}

void PlayerEngine::pause() {
    if (m_audioSink) m_audioSink->suspend();
}

void PlayerEngine::resume() {
    if (m_audioSink) m_audioSink->resume();
}

void PlayerEngine::seek(double seconds) {
    if (!m_currentProvider) return;
    m_baseOffsetSec = seconds;
    m_currentProvider->seek(seconds);
    if (!m_audioSink) return;
    QAudio::State oldState = m_audioSink->state();
    bool wasActive = (oldState == QAudio::ActiveState || oldState == QAudio::SuspendedState);
    if (!wasActive) return;
    m_audioSink->stop();
    delete m_audioSink;
    m_audioSink = nullptr;
    delete m_audioSource;
    m_audioSource = nullptr;
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    m_audioSource = new PlayerAudioSource(m_currentProvider, this);
    m_audioSink = new QAudioSink(device, m_currentFormat, this);
    int sampleRate = m_currentFormat.sampleRate();
    m_audioSink->setBufferSize(sampleRate * 2 * sizeof(float) * 0.1); 
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State s){
        if (s == QAudio::IdleState && m_audioSink->error() == QAudio::NoError) {
            m_audioSink->stop(); 
            emit playbackStopped();
        }
    });
    m_audioSink->start(m_audioSource);
    if (oldState == QAudio::SuspendedState) {
        m_audioSink->suspend();
    }
}

void PlayerEngine::onDefaultDeviceChanged(const QAudioDevice &device) {
    if (!m_currentProvider) return;
    QAudio::State oldState = QAudio::StoppedState;
    if (m_audioSink) {
        oldState = m_audioSink->state();
    }
    bool wasActive = (oldState == QAudio::ActiveState || oldState == QAudio::SuspendedState);
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    if (!wasActive) return; 
    m_baseOffsetSec = m_currentProvider->currentTime();
    m_audioSource = new PlayerAudioSource(m_currentProvider, this);
    m_audioSink = new QAudioSink(device, m_currentFormat, this);
    int sampleRate = m_currentFormat.sampleRate();
    m_audioSink->setBufferSize(sampleRate * 2 * sizeof(float) * 0.1);
    connect(m_audioSink, &QAudioSink::stateChanged, this, [this](QAudio::State s){
        if (s == QAudio::IdleState && m_audioSink->error() == QAudio::NoError) {
            m_audioSink->stop(); 
            emit playbackStopped();
        }
    });
    m_audioSink->start(m_audioSource);
    if (oldState == QAudio::SuspendedState) {
        m_audioSink->suspend();
    }
}