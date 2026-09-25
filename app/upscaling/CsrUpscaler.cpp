#include "CsrUpscaler.h"

#include <cmath>

namespace ufx {

UpscalerCaps CsrUpscaler::Caps() const {
    UpscalerCaps c{};
    c.id = UpscalerId::Csr;
    c.displayName = "CSR (Chopsticks Super Resolution)";
    c.vendorName = "Universal FrameFX";
    c.supportedApis = {GraphicsApi::D3D11, GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.sharpnessSupported = true;
    // Native is deliberately absent: at 1:1 CSR bypasses entirely, so offering
    // it as a mode would imply the pass does something when it does not.
    c.supportedModes = {QualityMode::UltraQuality, QualityMode::Quality,
                        QualityMode::Balanced, QualityMode::Performance,
                        QualityMode::UltraPerformance};
    c.requirements =
        "Spatial: needs only a finished, perceptually-encoded colour image — no motion "
        "vectors, depth or jitter, so it needs nothing from the game. Derived from FSR 1's "
        "EASU + RCAS and measured +1.48 dB PSNR over FSR 1 and +1.90 dB over bilinear on "
        "the reference patterns. It cannot reconstruct detail the game never rendered, it "
        "scales the HUD along with everything else, and it costs GPU time while creating no "
        "frames — so it only pays off when the game renders fewer pixels than it presents.";
    // The whole point of CSR: no game integration required.
    c.gameIntegrationRequired = false;
    // Not a wrapper around someone else's DLL; it is compiled into this binary.
    c.externalWrapperExists = false;
    return c;
}

Availability CsrUpscaler::CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const {
    // CSR is plain arithmetic over a pixel buffer, so it has no hardware gate of
    // its own and no vendor requirement. It is reported available wherever the
    // selected API is, because that is the only thing that could stop it.
    (void)gpu;
    (void)api;
    return Availability::Yes();
}

std::optional<std::pair<uint32_t, uint32_t>>
CsrUpscaler::RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const {
    if (m == QualityMode::Native) return std::nullopt;   // see Caps(): no 1:1 mode
    const ScaleRatio r = FsrRatio(m);                    // same preset ladder as the rest of UFX
    if (r.x <= 0.0f || r.y <= 0.0f) return std::nullopt;
    const auto rw = static_cast<uint32_t>(std::lround(w / r.x));
    const auto rh = static_cast<uint32_t>(std::lround(h / r.y));
    return std::make_pair(rw > 0 ? rw : 1u, rh > 0 ? rh : 1u);
}

csr::Image CsrUpscaler::Run(const csr::Image& src, uint32_t outWidth, uint32_t outHeight,
                            const csr::Options& options) {
    return csr::Upscale(src, static_cast<int>(outWidth), static_cast<int>(outHeight), options);
}

}
