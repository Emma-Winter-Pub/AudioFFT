#pragma once

#include "PlayerController.h"

#include <QString>
#include <functional>

class XPlayerSystemControls {
public:
    virtual ~XPlayerSystemControls() = default;
    using CommandCallback = std::function<void()>;
    virtual void setPlayCallback(CommandCallback cb) = 0;
    virtual void setPauseCallback(CommandCallback cb) = 0;
    virtual void setStopCallback(CommandCallback cb) = 0;
    virtual void updateState(PlayerController::State state) = 0;
    virtual void updateMediaInfo(const QString& title, const QString& artist) = 0;
    virtual void clear() = 0;
};