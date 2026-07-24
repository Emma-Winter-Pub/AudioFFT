#pragma once

#include "XPlayerSystemControls.h"

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

class PlayerLinuxMprisControls;

class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ CanQuit)
    Q_PROPERTY(bool Fullscreen READ Fullscreen)
    Q_PROPERTY(bool CanSetFullscreen READ CanSetFullscreen)
    Q_PROPERTY(bool CanRaise READ CanRaise)
    Q_PROPERTY(bool HasTrackList READ HasTrackList)
    Q_PROPERTY(QString Identity READ Identity)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)
public:
    explicit MprisRootAdaptor(QObject* parent) : QDBusAbstractAdaptor(parent) {}
    bool CanQuit() const { return false; }
    bool Fullscreen() const { return false; }
    bool CanSetFullscreen() const { return false; }
    bool CanRaise() const { return false; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return "AudioFFT"; }
    QString DesktopEntry() const { return "AudioFFT"; }
    QStringList SupportedUriSchemes() const { return {"file"}; }
    QStringList SupportedMimeTypes() const { return {}; }
public slots:
    void Quit() {}
    void Raise() {}
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus)
    Q_PROPERTY(QString LoopStatus READ LoopStatus)
    Q_PROPERTY(double Rate READ Rate)
    Q_PROPERTY(bool Shuffle READ Shuffle)
    Q_PROPERTY(QVariantMap Metadata READ Metadata)
    Q_PROPERTY(double Volume READ Volume)
    Q_PROPERTY(qlonglong Position READ Position)
    Q_PROPERTY(double MinimumRate READ MinimumRate)
    Q_PROPERTY(double MaximumRate READ MaximumRate)
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    Q_PROPERTY(bool CanPlay READ CanPlay)
    Q_PROPERTY(bool CanPause READ CanPause)
    Q_PROPERTY(bool CanSeek READ CanSeek)
    Q_PROPERTY(bool CanControl READ CanControl)
public:
    explicit MprisPlayerAdaptor(PlayerLinuxMprisControls* parent);
    QString PlaybackStatus() const;
    QString LoopStatus() const { return "None"; }
    double Rate() const { return 1.0; }
    bool Shuffle() const { return false; }
    QVariantMap Metadata() const;
    double Volume() const { return 1.0; }
    qlonglong Position() const { return 0; }
    double MinimumRate() const { return 1.0; }
    double MaximumRate() const { return 1.0; }
    bool CanGoNext() const { return false; }
    bool CanGoPrevious() const { return false; }
    bool CanPlay() const { return true; }
    bool CanPause() const { return true; }
    bool CanSeek() const { return false; }
    bool CanControl() const { return true; }
public slots:
    void Next() {}
    void Previous() {}
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong) {}
    void SetPosition(const QDBusObjectPath&, qlonglong) {}
    void OpenUri(const QString&) {}
private:
    PlayerLinuxMprisControls* m_parent;
};

class PlayerLinuxMprisControls : public QObject, public XPlayerSystemControls {
    Q_OBJECT
public:
    explicit PlayerLinuxMprisControls(QObject* parent = nullptr);
    ~PlayerLinuxMprisControls() override;
    void setPlayCallback(CommandCallback cb) override { m_playCb = std::move(cb); }
    void setPauseCallback(CommandCallback cb) override { m_pauseCb = std::move(cb); }
    void setStopCallback(CommandCallback cb) override { m_stopCb = std::move(cb); }
    void updateState(PlayerController::State state) override;
    void updateMediaInfo(const QString& title, const QString& artist) override;
    void clear() override;
    void mprisPlay();
    void mprisPause();
    void mprisStop();
    QString playbackStatus() const;
    QVariantMap metadata() const;

private:
    void emitPropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties);
    CommandCallback m_playCb;
    CommandCallback m_pauseCb;
    CommandCallback m_stopCb;
    PlayerController::State m_currentState = PlayerController::Stopped;
    QString m_title;
    QString m_artist;
    QString m_serviceName;
};