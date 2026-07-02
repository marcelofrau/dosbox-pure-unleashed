#pragma once

#include "..\Common\DeviceResources.h"
#include <cstdint>
#include <wrl.h>

namespace dosbox_uwp
{
    class RetroScreenRenderer
    {
    public:
        RetroScreenRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();
        void UpdateVideoFrame(const uint8_t* data, unsigned width, unsigned height, unsigned pitch);
        void Render();

    private:
        void RecreateBitmap(unsigned width, unsigned height);

        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_videoBitmap;
        unsigned m_frameWidth = 0;
        unsigned m_frameHeight = 0;
    };
}
