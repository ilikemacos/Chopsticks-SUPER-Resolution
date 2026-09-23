#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ufx {

enum class GpuVendor {
    Unknown,
    Nvidia,
    Amd,
    Intel,
    Microsoft, // WARP / basic render driver
};

// Coarse architecture family used for capability decisions.
// Never used to hide UI; only to compute *default* recommendations.
enum class GpuArch {
    Unknown,
    // AMD
    Gcn, Rdna1, Rdna2, Rdna3, Rdna4,
    // NVIDIA
    Pascal, Turing, Ampere, Ada, Blackwell,
    // Intel
    XeLp, XeHpg, XeHpc, Xe2,
};

struct GpuInfo {
    std::string name;
    GpuVendor vendor = GpuVendor::Unknown;
    GpuArch arch = GpuArch::Unknown;
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    uint64_t dedicatedVramBytes = 0;
    uint64_t sharedRamBytes = 0;
    std::string driverVersion;      // e.g. "31.0.15.4601"
    std::string driverVendor;       // e.g. "NVIDIA", "Advanced Micro Devices, Inc.", "Intel Corporation"
    bool supportsD3D11 = false;
    bool supportsD3D12 = false;
    // D3D_FEATURE_LEVEL encoded as its numeric value (e.g. 0xC000 == 12_0).
    uint32_t d3d11FeatureLevel = 0;
    uint32_t d3d12FeatureLevel = 0;
    bool supportsVulkan = false;
    std::string vulkanApiVersion;   // e.g. "1.3.275"
    bool isSoftware = false;
};

// Best-effort vendor lookup by PCI vendor id.
constexpr GpuVendor VendorFromId(uint32_t id) {
    switch (id) {
        case 0x10DE: return GpuVendor::Nvidia;
        case 0x1002: case 0x1022: return GpuVendor::Amd;
        case 0x8086: return GpuVendor::Intel;
        case 0x1414: return GpuVendor::Microsoft;
        default:     return GpuVendor::Unknown;
    }
}

const char* VendorName(GpuVendor v);
const char* ArchName(GpuArch a);

}
