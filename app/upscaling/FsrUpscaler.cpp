#include "FsrUpscaler.h"

#include <cmath>

namespace ufx {

FsrUpscaler::FsrUpscaler(int majorVersion) : version_(majorVersion) {}

UpscalerCaps FsrUpscaler::Caps() const {
    UpscalerCaps c{};
    c.vendorName = "AMD FidelityFX";
    c.supportedApis = {GraphicsApi::D3D11, GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.sharpnessSupported = true;

    switch (version_) {
        case 1:
            c.id = UpscalerId::Fsr1;
            c.displayName = "FSR 1 (spatial)";
            c.supportedModes = {QualityMode::UltraQuality, QualityMode::Quality,
                                QualityMode::Balanced, QualityMode::Performance};
            c.requirements = "Spatial upscaler; needs an aliased, post-tonemap color image only.";
            c.gameIntegrationRequired = true;  // still needs a render target hook
            c.externalWrapperExists = true;    // reshade-style spatial pass exists
            break;
        case 2:
            c.id = UpscalerId::Fsr2;
            c.displayName = "FSR 2";
            c.supportedModes = {QualityMode::Quality, QualityMode::Balanced,
                                QualityMode::Performance, QualityMode::UltraPerformance};
            c.requirements = "Temporal: motion vectors, depth and jitter offsets from the engine.";
            c.gameIntegrationRequired = true;
            c.externalWrapperExists = true;    // DLSS<->FSR2 wrappers exist for some games
            break;
        case 3:
            c.id = UpscalerId::Fsr3;
            c.displayName = "FSR 3 (upscaling)";
            c.supportedModes = {QualityMode::Native, QualityMode::Quality, QualityMode::Balanced,
                                QualityMode::Performance, QualityMode::UltraPerformance};
            c.requirements = "Temporal: motion vectors, depth and jitter from the engine. "
                             "Frame generation is a separate component (see Frame Generation).";
            c.gameIntegrationRequired = true;
            c.externalWrapperExists = true;
            break;
        case 4:
            c.id = UpscalerId::Fsr4;
            c.displayName = "FSR 4";
            c.supportedModes = {QualityMode::Quality, QualityMode::Balanced,
                                QualityMode::Performance};
            c.requirements = "ML-based; requires RDNA 4 hardware and a game that ships FSR 4.";
            c.gameIntegrationRequired = true;
            c.externalWrapperExists = false;
            break;
        default:
            c.id = UpscalerId::Fsr3;
            c.displayName = "FSR";
            break;
    }
    return c;
}

Availability FsrUpscaler::CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const {
    // FSR 1/2/3 are cross-vendor and run on any D3D12/Vulkan-capable GPU; the real
    // gate is always the *game*, which UFX cannot supply. We report hardware
    // eligibility here and surface the game-integration caveat in the UI.
    if (api == GraphicsApi::D3D12 && !gpu.supportsD3D12)
        return Availability::No("This GPU does not expose Direct3D 12.");
    if (api == GraphicsApi::D3D11 && !gpu.supportsD3D11)
        return Availability::No("This GPU does not expose Direct3D 11.");
    if (api == GraphicsApi::Vulkan && !gpu.supportsVulkan)
        return Availability::No("Vulkan is not available on this system.");

    if (version_ == 4) {
        if (gpu.vendor != GpuVendor::Amd)
            return Availability::No("FSR 4 requires an AMD RDNA 4 GPU; this system does not have one.");
        if (gpu.arch != GpuArch::Rdna4)
            return Availability::No("FSR 4 requires RDNA 4 hardware. Detected: "
                                    + std::string(ArchName(gpu.arch)) + ".");
        return Availability::Yes();
    }

    if (version_ == 1) return Availability::Yes();

    // FSR 2/3: FL 11_0+ is enough for the shader workload.
    return Availability::Yes();
}

std::optional<std::pair<uint32_t, uint32_t>>
FsrUpscaler::RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const {
    const auto& modes = Caps().supportedModes;
    bool ok = false;
    for (auto mm : modes) if (mm == m) { ok = true; break; }
    if (!ok) return std::nullopt;

    ScaleRatio r = FsrRatio(m);
    auto rw = static_cast<uint32_t>(std::lround(w / r.x));
    auto rh = static_cast<uint32_t>(std::lround(h / r.y));
    return std::make_pair(rw, rh);
}

}
