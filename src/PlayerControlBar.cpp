#include "PlayerControlBar.h"

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
    m_btnPlay->setToolTip(tr("播放/暂停 (Space)"));
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
    m_btnPlay->setToolTip(tr("播放/暂停 (Space)"));
    m_btnStop->setToolTip(tr("停止"));
}

void PlayerControlBar::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void PlayerControlBar::setProvider(int workspaceIndex, QSharedPointer<IPlayerProvider> provider, double totalDuration, bool isCurrentActive) {
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
    this->hide();
}

void PlayerControlBar::setPlayerFrameRate(int fps) {
    if (m_controller) {
        m_controller->setUpdateRate(fps);
    }
}

void PlayerControlBar::onPlayClicked() {
    if (m_controller->getState() == PlayerController::Playing) { 
        m_controller->pause(); 
    } else { 
        m_controller->play(); 
    }
}

void PlayerControlBar::onStopClicked() { m_controller->stop(); }

void PlayerControlBar::onControllerStateChanged(PlayerController::State state) {
    if (state == PlayerController::Playing) { 
        m_btnPlay->setSymbolType(SymbolType::Pause);
    } else { 
        m_btnPlay->setSymbolType(SymbolType::Play);
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