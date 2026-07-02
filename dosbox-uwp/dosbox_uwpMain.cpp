#include "pch.h"
#include "dosbox_uwpMain.h"
#include "Common\DirectXHelper.h"
#include <cmath>
#include <sstream>

using namespace dosbox_uwp;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

dosbox_uwpMain::dosbox_uwpMain(const std::shared_ptr<DX::DeviceResources>& deviceResources)
    : m_deviceResources(deviceResources)
    , m_clearColor(DirectX::Colors::CornflowerBlue)
    , m_clearTimer(0.0f)
    , m_lastButton(-1)
    , m_hasController(false)
    , m_eventText(L"")
    , m_eventTimer(0)
{
    m_deviceResources->RegisterDeviceNotify(this);

    m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));
    m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

    // init SDL (controller + audio)
    m_sdlInput = std::unique_ptr<SdlInput>(new SdlInput());
    if (m_sdlInput->Initialize())
    {
        m_hasController = m_sdlInput->HasController();
        char buf[128];
        sprintf_s(buf, "SDL: controller=%s audio=%s\n",
            m_hasController ? "CONNECTED" : "NONE (SPACE=btnA)",
            m_sdlInput->IsAudioReady() ? "OK" : "FAIL");
        OutputDebugStringA(buf);
    }
}

dosbox_uwpMain::~dosbox_uwpMain()
{
    m_deviceResources->RegisterDeviceNotify(nullptr);
}

void dosbox_uwpMain::CreateWindowSizeDependentResources()
{
    m_sceneRenderer->CreateWindowSizeDependentResources();
}

void dosbox_uwpMain::Update()
{
    m_timer.Tick([&]()
    {
        m_sdlInput->PollEvents();

        // A button edge: pressed this frame → beep
        if (m_sdlInput->WasButtonJustPressed(BUTTON_A))
        {
            OutputDebugStringA("A button PRESSED\n");
            m_sdlInput->PlayBeep(880.0f, 0.15f, 0.5f);
        }

        // A button held → green bg, else → normal
        if (m_sdlInput->IsButtonHeld(BUTTON_A))
        {
            m_clearColor = DirectX::Colors::Green;
        }
        else
        {
            m_clearColor = DirectX::Colors::CornflowerBlue;
        }

        // build debug status string
        {
            const char* lastEvent = m_sdlInput->GetLastEventText();
            if (lastEvent && lastEvent[0])
            {
                m_eventText = L"";
                for (const char* p = lastEvent; *p; p++)
                    m_eventText += (wchar_t)*p;
                m_eventTimer = 60; // show for ~1 second at 60fps
            }

            const wchar_t* inputSrc = m_sdlInput->HasControllerSDL() ? L"SDL" :
                m_sdlInput->HasControllerUWP() ? L"UWP" : L"KB";

            wchar_t buf[512];
            swprintf_s(buf, L"SDL:%s CTL:%s AUDIO:%s INP:%s\n"
                L"CTLR:%hs SND:%dHz BTN_A:%s  %s",
                m_sdlInput->IsInitialized() ? L"OK" : L"FAIL",
                m_hasController ? L"CONN" : L"NONE",
                m_sdlInput->IsAudioReady() ? L"OK" : L"FAIL",
                inputSrc,
                m_sdlInput->GetControllerName(),
                m_sdlInput->GetAudioSampleRate(),
                m_sdlInput->IsButtonHeld(BUTTON_A) ? L"HELD" : L"UP",
                (m_eventTimer > 0) ? m_eventText.c_str() : L"");
            if (m_eventTimer > 0) m_eventTimer--;
            m_fpsTextRenderer->SetDebugText(buf);
        }

        m_sceneRenderer->Update(m_timer);
        m_fpsTextRenderer->Update(m_timer);
    });
}

bool dosbox_uwpMain::Render()
{
    if (m_timer.GetFrameCount() == 0)
    {
        return false;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), m_clearColor);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_sceneRenderer->Render();
    m_fpsTextRenderer->Render();

    return true;
}

void dosbox_uwpMain::OnKeyEvent(Windows::System::VirtualKey key, bool down)
{
    if (key == Windows::System::VirtualKey::Space)
    {
        m_sdlInput->SetKeyboardButton(BUTTON_A, down);
        if (down) m_sdlInput->PlayBeep(880.0f, 0.15f, 0.5f);
    }
}

void dosbox_uwpMain::OnDeviceLost()
{
    m_sceneRenderer->ReleaseDeviceDependentResources();
    m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

void dosbox_uwpMain::OnDeviceRestored()
{
    m_sceneRenderer->CreateDeviceDependentResources();
    m_fpsTextRenderer->CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
