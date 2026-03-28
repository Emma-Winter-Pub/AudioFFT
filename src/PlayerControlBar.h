#pragma once

#include "PlayerController.h"
#include "IPlayerProvider.h"
#include "PlayerControlComponents.h" 

#include <QWidget>
#include <QLabel>
#include <QSharedPointer>
#include <QMap>
#include <QEvent>

struct PlayerWorkspaceState {
    QSharedPointer<IPlayerProvider> provider;
    double savedTime = 0.0;
};

class PlayerControlBar : public QWidget {
    Q_OBJECT

public:
    explicit PlayerControlBar(QWidget *parent = nullptr);
    ~PlayerControlBar();
    void switchWorkspace(int oldIndex, int newIndex);
    void setProvider(int workspaceIndex, QSharedPointer<IPlayerProvider> provider, double totalDuration, bool isCurrentActive);
    void seek(double seconds);
    void stopAndClear();
    void setPlayerFrameRate(int fps);

signals:
    void timeChanged(double seconds);
    void stateChanged(PlayerController::State state);

protected:
    void changeEvent(QEvent *event) override;

public slots:
    void onPlayClicked();

private slots:
    void onStopClicked();
    void onControllerStateChanged(PlayerController::State state);
    void onControllerTimeChanged(double seconds);

private:
    void setupUi();
    void retranslateUi();
    QString formatTime(double seconds) const;
    QLabel* m_lblTime = nullptr;
    SymbolButton* m_btnPlay = nullptr;
    SymbolButton* m_btnStop = nullptr;
    PlayerController* m_controller = nullptr;
    QMap<int, PlayerWorkspaceState> m_states;
};