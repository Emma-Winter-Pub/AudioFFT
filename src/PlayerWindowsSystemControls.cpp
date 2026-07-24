#include "PlayerWindowsSystemControls.h"

#include <wrl/wrappers/corewrappers.h>
#include <systemmediatransportcontrolsinterop.h>

#pragma comment(lib, "runtimeobject.lib")

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Foundation;

PlayerWindowsSystemControls::PlayerWindowsSystemControls(HWND hwnd) {
    HStringReference classId(RuntimeClass_Windows_Media_SystemMediaTransportControls);
    ComPtr<ISystemMediaTransportControlsInterop> interop;
    if (FAILED(RoGetActivationFactory(classId.Get(), IID_PPV_ARGS(&interop)))) return;
    if (FAILED(interop->GetForWindow(hwnd, IID_PPV_ARGS(&m_smtc))) || !m_smtc) return;
    m_smtc->put_IsEnabled(true);
    m_smtc->put_IsPlayEnabled(true);
    m_smtc->put_IsPauseEnabled(true);
    m_smtc->put_IsStopEnabled(true);
    auto handler = Callback<ITypedEventHandler<SystemMediaTransportControls*, SystemMediaTransportControlsButtonPressedEventArgs*>>(
        [this](ISystemMediaTransportControls*, ISystemMediaTransportControlsButtonPressedEventArgs* args) -> HRESULT {
            SystemMediaTransportControlsButton button;
            if (SUCCEEDED(args->get_Button(&button))) {
                handleButtonPress(button);
            }
            return S_OK;
        });
    m_smtc->add_ButtonPressed(handler.Get(), &m_buttonPressedToken);
}

PlayerWindowsSystemControls::~PlayerWindowsSystemControls() {
    if (m_smtc) {
        if (m_buttonPressedToken.value != 0) {
            m_smtc->remove_ButtonPressed(m_buttonPressedToken);
        }
        m_smtc->put_IsEnabled(false);
        m_smtc->put_PlaybackStatus(MediaPlaybackStatus_Closed);
    }
}

void PlayerWindowsSystemControls::updateMediaInfo(const QString& title, const QString& artist) {
    if (!m_smtc) return;
    ComPtr<ISystemMediaTransportControlsDisplayUpdater> updater;
    if (SUCCEEDED(m_smtc->get_DisplayUpdater(&updater)) && updater) {
        updater->put_Type(MediaPlaybackType_Music);
        ComPtr<IMusicDisplayProperties> musicProps;
        if (SUCCEEDED(updater->get_MusicProperties(&musicProps)) && musicProps) {
            HString hTitle;
            hTitle.Set(reinterpret_cast<const WCHAR*>(title.utf16()), title.length());
            musicProps->put_Title(hTitle.Get());
            HString hArtist;
            hArtist.Set(reinterpret_cast<const WCHAR*>(artist.utf16()), artist.length());
            musicProps->put_Artist(hArtist.Get());
        }
        updater->Update();
    }
}

void PlayerWindowsSystemControls::updateState(PlayerController::State state) {
    if (!m_smtc) return;
    MediaPlaybackStatus status = MediaPlaybackStatus_Closed;
    if (state == PlayerController::Playing) {
        status = MediaPlaybackStatus_Playing;
    } else if (state == PlayerController::Paused) {
        status = MediaPlaybackStatus_Paused;
    } else if (state == PlayerController::Stopped) {
        status = MediaPlaybackStatus_Stopped;
    }
    m_smtc->put_PlaybackStatus(status);
}

void PlayerWindowsSystemControls::clear() {
    if (m_smtc) {
        m_smtc->put_PlaybackStatus(MediaPlaybackStatus_Closed);
    }
}

void PlayerWindowsSystemControls::handleButtonPress(SystemMediaTransportControlsButton button) {
    switch (button) {
        case SystemMediaTransportControlsButton_Play:
            if (m_playCb) m_playCb();
            break;
        case SystemMediaTransportControlsButton_Pause:
            if (m_pauseCb) m_pauseCb();
            break;
        case SystemMediaTransportControlsButton_Stop:
            if (m_stopCb) m_stopCb();
            break;
        default: 
            break;
    }
}