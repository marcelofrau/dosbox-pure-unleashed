#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"
#include "Content\SdlInput.h"

namespace dosbox_uwp
{
    class dosbox_uwpMain : public DX::IDeviceNotify
    {
    public:
        dosbox_uwpMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        ~dosbox_uwpMain();
        void CreateWindowSizeDependentResources();
        void Update();
        bool Render();

        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();
        void OnKeyEvent(Windows::System::VirtualKey key, bool down);

    private:
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
        std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;
        std::unique_ptr<SdlInput> m_sdlInput;

        DX::StepTimer m_timer;

        DirectX::XMVECTORF32 m_clearColor;
        float m_clearTimer;
        int m_lastButton;
        bool m_hasController;
        std::wstring m_eventText;
        int m_eventTimer;
    };
}
