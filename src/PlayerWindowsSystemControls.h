#pragma once

#include "XPlayerSystemControls.h"

#include <windows.h>
#include <wrl.h>
#include <windows.media.h>

class PlayerWindowsSystemControls : public XPlayerSystemControls {
public:
    explicit PlayerWindowsSystemControls(HWND hwnd);
    ~PlayerWindowsSystemControls() override;
    void setPlayCallback(CommandCallback cb) override { m_playCb = std::move(cb); }
    void setPauseCallback(CommandCallback cb) override { m_pauseCb = std::move(cb); }
    void setStopCallback(CommandCallback cb) override { m_stopCb = std::move(cb); }
    void updateState(PlayerController::State state) override;
    void updateMediaInfo(const QString& title, const QString& artist) override;
    void clear() override;

private:
    void handleButtonPress(ABI::Windows::Media::SystemMediaTransportControlsButton button);
    Microsoft::WRL::ComPtr<ABI::Windows::Media::ISystemMediaTransportControls> m_smtc;
    EventRegistrationToken m_buttonPressedToken{};
    CommandCallback m_playCb;
    CommandCallback m_pauseCb;
    CommandCallback m_stopCb;
};