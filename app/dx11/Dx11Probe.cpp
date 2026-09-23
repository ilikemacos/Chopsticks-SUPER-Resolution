#include "Dx11Probe.h"

#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ufx {

Dx11Caps ProbeDx11() {
    Dx11Caps caps{};
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL got{};
    ComPtr<ID3D11Device> dev;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                   levels, static_cast<UINT>(std::size(levels)),
                                   D3D11_SDK_VERSION, &dev, &got, nullptr);
    if (SUCCEEDED(hr)) {
        caps.available = true;
        caps.featureLevel = static_cast<unsigned>(got);
    }
    ComPtr<IDXGIFactory5> factory;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        BOOL tearing = FALSE;
        if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                   &tearing, sizeof(tearing)))) {
            caps.tearingSupported = tearing == TRUE;
        }
    }
    return caps;
}

}
#endif
