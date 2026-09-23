#include "NullUpscaler.h"

namespace ufx {

UpscalerCaps NullUpscaler::Caps() const {
    UpscalerCaps c{};
    c.id = UpscalerId::None;
    c.displayName = "Native (no upscaling)";
    c.vendorName = "-";
    c.supportedApis = {GraphicsApi::D3D11, GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.supportedModes = {QualityMode::Native};
    c.sharpnessSupported = false;
    c.requirements = "None. The game renders at its native output resolution.";
    c.gameIntegrationRequired = false;
    c.externalWrapperExists = false;
    return c;
}

Availability NullUpscaler::CheckAvailability(const GpuInfo&, GraphicsApi) const {
    return Availability::Yes();
}

std::optional<std::pair<uint32_t, uint32_t>>
NullUpscaler::RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const {
    if (m != QualityMode::Native) return std::nullopt;
    return std::make_pair(w, h);
}

}
