#include "PlayerControlBar.h"

#ifdef Q_OS_WIN
#include "PlayerWindowsSystemControls.h"
#elif defined(Q_OS_LINUX)
#include "PlayerLinuxMprisControls.h"
#endif

#include <QPointer>
#include <QHBoxLayout>
#include <QDebug>

PlayerControlBar::PlayerControlBar(QWidget *parent)
    : QWidget(parent)
{
    m_controller = new PlayerController(this);
    connect(m_controller, &PlayerController::stateChanged, this, &PlayerControlBar::onControllerStateChanged);
    connect(m_controller, &PlayerController::timeChanged, this, &PlayerControlBar::onControllerTimeChanged);
    setupUi();
    retranslateUi();

#ifdef Q_OS_WIN
    HWND topLevelHwnd = reinterpret_cast<HWND>(this->window()->winId());
    m_systemControls = std::make_unique<PlayerWindowsSystemControls>(topLevelHwnd);
#elif defined(Q_OS_LINUX)
    m_systemControls = std::make_unique<PlayerLinuxMprisControls>(this);
#endif

    if (m_systemControls) {
        QPointer<PlayerControlBar> safeThis(this);
        m_systemControls->setPlayCallback([safeThis]() {
            if (safeThis) {
                QMetaObject::invokeMethod(safeThis.data(), &PlayerControlBar::play, Qt::QueuedConnection);
            }
        });
        m_systemControls->setPauseCallback([safeThis]() {
            if (safeThis) {
                QMetaObject::invokeMethod(safeThis.data(), [safeThis]() {
                    if (safeThis->m_controller && safeThis->m_controller->getState() == PlayerController::Playing) {
                        safeThis->m_controller->pause();
                    }
                }, Qt::QueuedConnection);
            }
        });
        m_systemControls->setStopCallback([safeThis]() {
            if (safeThis) {
                QMetaObject::invokeMethod(safeThis.data(), &PlayerControlBar::stopAndClear, Qt::QueuedConnection);
            }
        });
    }
}

PlayerControlBar::~PlayerControlBar() {
    if (m_controller) m_controller->stop();
}

void PlayerControlBar::setupUi() {
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    m_lblTime = new QLabel("00:00:00.00", this);
    m_lblTime->setStyleSheet(
        "QLabel {"
        "   font-size: 9pt;"                 
        "   color: #4CC2FF;"      
        "   margin-right: 8px;"              
        "}"
    );
    m_lblTime->setFixedWidth(80);            
    m_lblTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_btnPlay = new SymbolButton(this);
    m_btnPlay->setSymbolType(SymbolType::Play);
    m_btnPlay->setToolTip(tr("播放/暂停 (空格键)"));
    m_btnPlay->setFocusPolicy(Qt::NoFocus); 
    connect(m_btnPlay, &SymbolButton::clicked, this, &PlayerControlBar::onPlayClicked);
    m_btnStop = new SymbolButton(this);
    m_btnStop->setSymbolType(SymbolType::Stop);
    m_btnStop->setToolTip(tr("停止"));
    m_btnStop->setFocusPolicy(Qt::NoFocus);
    connect(m_btnStop, &SymbolButton::clicked, this, &PlayerControlBar::onStopClicked);
    layout->addWidget(m_lblTime);
    layout->addWidget(m_btnPlay);
    layout->addWidget(m_btnStop);
    this->hide();
}

void PlayerControlBar::retranslateUi() {
    m_btnPlay->setToolTip(tr("播放/暂停 (空格键)"));
    m_btnStop->setToolTip(tr("停止"));
}

void PlayerControlBar::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void PlayerControlBar::setProvider(int workspaceIndex, QSharedPointer<XPlayerProvider> provider, double totalDuration, bool isCurrentActive) {
    Q_UNUSED(totalDuration);
    PlayerWorkspaceState& state = m_states[workspaceIndex];
    state.provider = provider;
    state.savedTime = 0.0;
    if (isCurrentActive) {
        m_controller->stop();
        m_controller->load(provider);
        onControllerTimeChanged(0.0);
        onControllerStateChanged(PlayerController::Stopped);
        this->show();
    }
}

void PlayerControlBar::switchWorkspace(int oldIndex, int newIndex) {
    if (m_states.contains(oldIndex)) {
        if (m_states[oldIndex].provider) {
            m_states[oldIndex].savedTime = m_controller->getCurrentTime(); 
        }
    }
    m_controller->pause();
    if (m_states.contains(newIndex) && m_states[newIndex].provider) {
        PlayerWorkspaceState& newState = m_states[newIndex];
        m_controller->load(newState.provider);
        newState.provider->seek(newState.savedTime);
        this->show();
        onControllerTimeChanged(newState.savedTime);
        onControllerStateChanged(PlayerController::Paused); 
    } else {
        m_controller->stop();
        this->hide(); 
    }
}

void PlayerControlBar::seek(double seconds) { m_controller->seek(seconds); }

void PlayerControlBar::stopAndClear() {
    m_controller->stop();
    m_controller->seek(0.0);
    this->hide();
    if (m_systemControls) {
        m_systemControls->clear();
    }
}

void PlayerControlBar::setPlayerFrameRate(int fps) {
    if (m_controller) {
        m_controller->setUpdateRate(fps);
    }
}

void PlayerControlBar::play() {
    if (m_controller && m_controller->getState() != PlayerController::Playing) {
        m_controller->play();
    }
}

void PlayerControlBar::onPlayClicked() {
    if (m_controller->getState() == PlayerController::Playing) { 
        m_controller->pause(); 
    } else { 
        m_controller->play(); 
    }
}

void PlayerControlBar::onStopClicked() { 
    m_controller->stop(); 
    m_controller->seek(0.0);
}

void PlayerControlBar::onControllerStateChanged(PlayerController::State state) {
    if (state == PlayerController::Playing) { 
        m_btnPlay->setSymbolType(SymbolType::Pause);
    } else { 
        m_btnPlay->setSymbolType(SymbolType::Play);
    }
    if (m_systemControls) {
        m_systemControls->updateState(state);
    }
    emit stateChanged(state);
}

void PlayerControlBar::onControllerTimeChanged(double seconds) {
    m_lblTime->setText(formatTime(seconds));
    emit timeChanged(seconds);
}

QString PlayerControlBar::formatTime(double seconds) const {
    long long totalCentiseconds = static_cast<long long>(seconds * 100.0);
    int cs = totalCentiseconds % 100;
    long long totalSec = totalCentiseconds / 100;
    int sec = totalSec % 60;
    int min = (totalSec / 60) % 60;
    int hour = totalSec / 3600;
    if (hour > 0) return QString("%1:%2:%3.%4").arg(hour).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(cs, 2, 10, QChar('0'));
    else return QString("%1:%2.%3").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(cs, 2, 10, QChar('0'));
}

void PlayerControlBar::updateMediaInfo(const QString& title, const QString& artist) {
    if (m_systemControls) {
        m_systemControls->updateMediaInfo(title, artist);
    }
}