#include "Dx12Probe.h"

#ifdef _WIN32
#include <windows.h>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ufx {

Dx12Caps ProbeDx12() {
    Dx12Caps caps{};
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    };
    for (auto lv : levels) {
        ComPtr<ID3D12Device> dev;
        if (SUCCEEDED(D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&dev)))) {
            caps.available = true;
            caps.featureLevel = static_cast<unsigned>(lv);

            D3D12_FEATURE_DATA_D3D12_OPTIONS7 opt7{};
            if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &opt7, sizeof(opt7)))) {
                caps.meshShaders = opt7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
            }
            D3D12_FEATURE_DATA_SHADER_MODEL sm{ D3D_SHADER_MODEL_6_6 };
            if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm)))) {
                caps.sm66 = sm.HighestShaderModel >= D3D_SHADER_MODEL_6_6;
            }
            D3D12_FEATURE_DATA_D3D12_OPTIONS1 opt1{};
            if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &opt1, sizeof(opt1)))) {
                caps.waveIntrinsics = opt1.WaveOps == TRUE;
            }
            D3D12_FEATURE_DATA_D3D12_OPTIONS12 opt12{};
            if (SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &opt12, sizeof(opt12)))) {
                caps.enhancedBarriers = opt12.EnhancedBarriersSupported == TRUE;
            }
            return caps;
        }
    }
    return caps;
}

}
#endif
