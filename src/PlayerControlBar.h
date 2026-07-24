#pragma once

#include "PlayerController.h"
#include "XPlayerProvider.h"
#include "PlayerControlComponents.h" 
#include "XPlayerSystemControls.h"

#include <memory>
#include <QWidget>
#include <QLabel>
#include <QSharedPointer>
#include <QMap>
#include <QEvent>

struct PlayerWorkspaceState {
    QSharedPointer<XPlayerProvider> provider;
    double savedTime = 0.0;
};

class PlayerControlBar : public QWidget {
    Q_OBJECT

public:
    explicit PlayerControlBar(QWidget *parent = nullptr);
    ~PlayerControlBar();
    void switchWorkspace(int oldIndex, int newIndex);
    void setProvider(int workspaceIndex, QSharedPointer<XPlayerProvider> provider, double totalDuration, bool isCurrentActive);
    void seek(double seconds);
    void stopAndClear();
    void setPlayerFrameRate(int fps);
    void play();
    void updateMediaInfo(const QString& title, const QString& artist);

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
    std::unique_ptr<XPlayerSystemControls> m_systemControls;
};