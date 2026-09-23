#include "XessUpscaler.h"

#include <cmath>

namespace ufx {

UpscalerCaps XessUpscaler::Caps() const {
    UpscalerCaps c{};
    c.id = UpscalerId::XeSS;
    c.displayName = "Intel XeSS";
    c.vendorName = "Intel";
    c.supportedApis = {GraphicsApi::D3D11, GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.supportedModes = {QualityMode::UltraQuality, QualityMode::Quality, QualityMode::Balanced,
                        QualityMode::Performance, QualityMode::UltraPerformance};
    c.sharpnessSupported = false; // XeSS handles sharpening internally
    c.requirements = "Temporal: motion vectors, depth and jitter from the engine. "
                     "Runs on Intel Arc (XMX) or any GPU via the DP4a fallback path.";
    c.gameIntegrationRequired = true;
    c.externalWrapperExists = true;   // DLSS<->XeSS wrappers exist for some titles
    return c;
}

Availability XessUpscaler::CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const {
    if (api == GraphicsApi::D3D12 && !gpu.supportsD3D12)
        return Availability::No("This GPU does not expose Direct3D 12.");
    if (api == GraphicsApi::D3D11 && !gpu.supportsD3D11)
        return Availability::No("This GPU does not expose Direct3D 11.");
    if (api == GraphicsApi::Vulkan && !gpu.supportsVulkan)
        return Availability::No("Vulkan is not available on this system.");

    // XeSS runs everywhere via DP4a; Intel Arc gets the faster XMX path.
    // Requires Shader Model 6.4 / DP4a which needs FL 12_0+ practically.
    if (gpu.vendor == GpuVendor::Intel && (gpu.arch == GpuArch::XeHpg || gpu.arch == GpuArch::Xe2))
        return Availability::Yes(); // XMX path

    return Availability::Yes(); // DP4a fallback path on other vendors
}

std::optional<std::pair<uint32_t, uint32_t>>
XessUpscaler::RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const {
    const auto& modes = Caps().supportedModes;
    bool ok = false;
    for (auto mm : modes) if (mm == m) { ok = true; break; }
    if (!ok) return std::nullopt;

    ScaleRatio r = XessRatio(m);
    auto rw = static_cast<uint32_t>(std::lround(w / r.x));
    auto rh = static_cast<uint32_t>(std::lround(h / r.y));
    return std::make_pair(rw, rh);
}

}
