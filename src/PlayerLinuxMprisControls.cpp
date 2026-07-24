#include "PlayerLinuxMprisControls.h"

#include <QCoreApplication>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>

MprisPlayerAdaptor::MprisPlayerAdaptor(PlayerLinuxMprisControls* parent) 
    : QDBusAbstractAdaptor(parent), m_parent(parent) {}

QString MprisPlayerAdaptor::PlaybackStatus() const { return m_parent->playbackStatus(); }
QVariantMap MprisPlayerAdaptor::Metadata() const { return m_parent->metadata(); }
void MprisPlayerAdaptor::Pause() { m_parent->mprisPause(); }
void MprisPlayerAdaptor::Stop() { m_parent->mprisStop(); }
void MprisPlayerAdaptor::Play() { m_parent->mprisPlay(); }
void MprisPlayerAdaptor::PlayPause() {
    if (PlaybackStatus() == "Playing") m_parent->mprisPause();
    else m_parent->mprisPlay();
}

PlayerLinuxMprisControls::PlayerLinuxMprisControls(QObject* parent)
    : QObject(parent)
{
    new MprisRootAdaptor(this);
    new MprisPlayerAdaptor(this);
    qint64 pid = QCoreApplication::applicationPid();
    m_serviceName = QString("org.mpris.MediaPlayer2.AudioFFT.instance%1").arg(pid);
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerService(m_serviceName);
    bus.registerObject("/org/mpris/MediaPlayer2", this);
}

PlayerLinuxMprisControls::~PlayerLinuxMprisControls() {
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject("/org/mpris/MediaPlayer2");
    bus.unregisterService(m_serviceName);
}

void PlayerLinuxMprisControls::updateState(PlayerController::State state) {
    if (m_currentState == state) return;
    m_currentState = state;
    QVariantMap changedProps;
    changedProps.insert("PlaybackStatus", playbackStatus());
    emitPropertiesChanged("org.mpris.MediaPlayer2.Player", changedProps);
}

void PlayerLinuxMprisControls::updateMediaInfo(const QString& title, const QString& artist) {
    m_title = title;
    m_artist = artist;
    QVariantMap changedProps;
    changedProps.insert("Metadata", metadata());
    emitPropertiesChanged("org.mpris.MediaPlayer2.Player", changedProps);
}

void PlayerLinuxMprisControls::clear() {
    m_title.clear();
    m_artist.clear();
    updateState(PlayerController::Stopped);
    QVariantMap changedProps;
    changedProps.insert("Metadata", metadata());
    emitPropertiesChanged("org.mpris.MediaPlayer2.Player", changedProps);
}

QString PlayerLinuxMprisControls::playbackStatus() const {
    switch (m_currentState) {
        case PlayerController::Playing: return "Playing";
        case PlayerController::Paused:  return "Paused";
        default:                        return "Stopped";
    }
}

QVariantMap PlayerLinuxMprisControls::metadata() const {
    QVariantMap meta;
    meta.insert("mpris:trackid", QVariant::fromValue(QDBusObjectPath("/org/mpris/MediaPlayer2/TrackList/NoTrack")));
    if (!m_title.isEmpty()) {
        meta.insert("xesam:title", m_title);
    }
    if (!m_artist.isEmpty()) {
        meta.insert("xesam:artist", QVariant::fromValue(QStringList() << m_artist));
    }
    return meta;
}

void PlayerLinuxMprisControls::emitPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties) {
    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged"
    );
    signal << interfaceName;
    signal << changedProperties;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

void PlayerLinuxMprisControls::mprisPlay() { if (m_playCb) m_playCb(); }
void PlayerLinuxMprisControls::mprisPause() { if (m_pauseCb) m_pauseCb(); }
void PlayerLinuxMprisControls::mprisStop() { if (m_stopCb) m_stopCb(); }