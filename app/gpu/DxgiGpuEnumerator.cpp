#include "DxgiGpuEnumerator.h"

#ifdef _WIN32
#include <windows.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d12.h>
#include <wrl/client.h>

#include <cstdio>
#include <string>

using Microsoft::WRL::ComPtr;

namespace ufx {

namespace {

std::string Narrow(const wchar_t* w) {
    if (!w) return {};
    int len = ::WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string out(len - 1, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), len, nullptr, nullptr);
    return out;
}

GpuArch GuessArch(GpuVendor v, uint32_t deviceId) {
    // Coarse best-effort mapping. Never used to hide UI.
    switch (v) {
        case GpuVendor::Nvidia:
            if (deviceId >= 0x2B00) return GpuArch::Blackwell;
            if (deviceId >= 0x2600) return GpuArch::Ada;
            if (deviceId >= 0x2200) return GpuArch::Ampere;
            if (deviceId >= 0x1E00) return GpuArch::Turing;
            return GpuArch::Pascal;
        case GpuVendor::Amd:
            if (deviceId >= 0x7500) return GpuArch::Rdna4;
            if (deviceId >= 0x7440) return GpuArch::Rdna3;
            if (deviceId >= 0x73A0) return GpuArch::Rdna2;
            if (deviceId >= 0x7310) return GpuArch::Rdna1;
            return GpuArch::Gcn;
        case GpuVendor::Intel:
            if (deviceId >= 0xE200) return GpuArch::Xe2;
            if (deviceId >= 0x5690) return GpuArch::XeHpg;
            return GpuArch::XeLp;
        default:
            return GpuArch::Unknown;
    }
}

std::string ProbeDriverVersion(const DXGI_ADAPTER_DESC1& desc) {
    LARGE_INTEGER umd{};
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return {};
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0;; ++i) {
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) return {};
        DXGI_ADAPTER_DESC1 d{};
        adapter->GetDesc1(&d);
        if (d.AdapterLuid.LowPart == desc.AdapterLuid.LowPart &&
            d.AdapterLuid.HighPart == desc.AdapterLuid.HighPart) {
            if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd))) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                              HIWORD(umd.HighPart), LOWORD(umd.HighPart),
                              HIWORD(umd.LowPart),  LOWORD(umd.LowPart));
                return buf;
            }
            return {};
        }
    }
}

void ProbeD3D11(GpuInfo& info, IDXGIAdapter1* adapter) {
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    };
    D3D_FEATURE_LEVEL got{};
    ComPtr<ID3D11Device> dev;
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                                   levels, static_cast<UINT>(std::size(levels)),
                                   D3D11_SDK_VERSION, &dev, &got, nullptr);
    if (SUCCEEDED(hr)) {
        info.supportsD3D11 = true;
        info.d3d11FeatureLevel = static_cast<uint32_t>(got);
    }
}

void ProbeD3D12(GpuInfo& info, IDXGIAdapter1* adapter) {
    static const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
    };
    for (auto lv : levels) {
        ComPtr<ID3D12Device> dev;
        if (SUCCEEDED(D3D12CreateDevice(adapter, lv, IID_PPV_ARGS(&dev)))) {
            info.supportsD3D12 = true;
            info.d3d12FeatureLevel = static_cast<uint32_t>(lv);
            return;
        }
    }
}

}

std::vector<GpuInfo> DxgiGpuEnumerator::Enumerate() {
    std::vector<GpuInfo> out;
    ComPtr<IDXGIFactory6> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return out;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);

        GpuInfo info{};
        info.name = Narrow(desc.Description);
        info.vendorId = desc.VendorId;
        info.deviceId = desc.DeviceId;
        info.vendor = VendorFromId(desc.VendorId);
        info.arch = GuessArch(info.vendor, desc.DeviceId);
        info.dedicatedVramBytes = desc.DedicatedVideoMemory;
        info.sharedRamBytes = desc.SharedSystemMemory;
        info.isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        info.driverVersion = ProbeDriverVersion(desc);
        info.driverVendor = VendorName(info.vendor);
        ProbeD3D11(info, adapter.Get());
        ProbeD3D12(info, adapter.Get());
        // Vulkan probing lives in vulkan/VulkanProbe to keep this file free of a Vulkan link.
        out.push_back(std::move(info));
    }
    return out;
}

}
#endif
