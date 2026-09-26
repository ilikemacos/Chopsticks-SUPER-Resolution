#include "FsrFrameGenerator.h"

namespace ufx {

FrameGenCaps FsrFrameGenerator::Caps() const {
    FrameGenCaps c{};
    c.id = FrameGenId::Fsr3Fg;
    c.displayName = "FSR 3 Frame Generation";
    c.vendorName = "AMD FidelityFX";
    c.supportedApis = {GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.requirements = "The game must integrate FidelityFX Frame Generation "
                     "(swapchain proxy, motion vectors, UI composition).";
    c.externalLimitation =
        "Universal FrameFX cannot add FSR 3 Frame Generation to a game that did not "
        "ship it. Frame generation intercepts the swapchain and needs engine motion "
        "vectors; there is no reliable external injection path. Where a game ships "
        "FSR 3 FG, UFX can toggle it in the game's own config.";
    return c;
}

FrameGenState FsrFrameGenerator::Evaluate(const GpuInfo& gpu, GraphicsApi api,
                                          bool gameDeclaresSupport) const {
    if (api == GraphicsApi::D3D11)
        return FrameGenState::RequiresCompatibleImpl; // FSR3 FG is D3D12/VK only
    if (!gpu.supportsD3D12 && api == GraphicsApi::D3D12)
        return FrameGenState::RequiresCompatibleImpl;
    if (!gameDeclaresSupport)
        return FrameGenState::RequiresGameIntegration;
    return FrameGenState::Supported;
}

FrameGenCaps XessFrameGenerator::Caps() const {
    FrameGenCaps c{};
    c.id = FrameGenId::XessFg;
    c.displayName = "XeSS Frame Generation";
    c.vendorName = "Intel";
    c.supportedApis = {GraphicsApi::D3D12};
    c.requirements = "The game must integrate XeSS Frame Generation. "
                     "Best on Intel Arc; other GPUs depend on the game's implementation.";
    c.externalLimitation =
        "Universal FrameFX cannot add XeSS Frame Generation to a game that did not ship "
        "it. Where a game integrates it, UFX exposes the game's own toggle.";
    return c;
}

FrameGenState XessFrameGenerator::Evaluate(const GpuInfo& gpu, GraphicsApi api,
                                           bool gameDeclaresSupport) const {
    if (api != GraphicsApi::D3D12)
        return FrameGenState::RequiresCompatibleImpl;
    if (!gpu.supportsD3D12)
        return FrameGenState::RequiresCompatibleImpl;
    if (!gameDeclaresSupport)
        return FrameGenState::RequiresGameIntegration;
    return FrameGenState::Supported;
}

FrameGenCaps CsrFrameGenerator::Caps() const {
    FrameGenCaps c{};
    c.id = FrameGenId::CsrInterpolated;
    c.displayName = "CSR Frame Generation (interpolation)";
    c.vendorName = "Universal FrameFX";
    // App-agnostic: it works on the frames any app presents, whatever API the
    // app renders with, because it captures the finished frames.
    c.supportedApis = {GraphicsApi::D3D11, GraphicsApi::D3D12, GraphicsApi::Vulkan};
    c.requirements = "Runs on the frames any app presents (Windows Graphics "
                     "Capture); no game integration or engine data needed.";
    c.externalLimitation =
        "CSR Frame Generation interpolates a synthetic frame between two captured "
        "frames. It raises DISPLAYED FPS but ADDS latency (a real frame is held to "
        "interpolate against) and cannot reduce it -- input responsiveness gets "
        "worse, not better. It smears on fast motion, disocclusion and HUD/UI, and "
        "is best above ~60 FPS. It is added smoothness, not more performance, and "
        "must never be presented as a real-FPS gain. Measured in csr/FRAMEGEN.md.";
    return c;
}

FrameGenState CsrFrameGenerator::Evaluate(const GpuInfo& gpu, GraphicsApi api,
                                          bool /*gameDeclaresSupport*/) const {
    // External and generic: it does not need the game to integrate anything, so
    // unlike FSR/XeSS FG it is not gated on gameDeclaresSupport. It needs a GPU
    // that can run the capture + compute path.
    (void)api;
    if (!gpu.supportsD3D11 && !gpu.supportsD3D12)
        return FrameGenState::RequiresCompatibleImpl;
    return FrameGenState::Supported;
}

const char* FrameGenStateLabel(FrameGenState s) {
    switch (s) {
        case FrameGenState::Disabled:                return "Disabled";
        case FrameGenState::Supported:               return "Supported";
        case FrameGenState::RequiresGameIntegration: return "Requires game integration";
        case FrameGenState::RequiresCompatibleImpl:  return "Requires compatible implementation";
    }
    return "Unknown";
}

const char* FrameGenIdName(FrameGenId id) {
    switch (id) {
        case FrameGenId::None:            return "None";
        case FrameGenId::Fsr3Fg:          return "FSR3-FG";
        case FrameGenId::XessFg:          return "XeSS-FG";
        case FrameGenId::CsrInterpolated: return "CSR-FG";
    }
    return "None";
}

bool ParseFrameGenId(std::string_view s, FrameGenId& out) {
    if (s == "None")    { out = FrameGenId::None;            return true; }
    if (s == "FSR3-FG") { out = FrameGenId::Fsr3Fg;         return true; }
    if (s == "XeSS-FG") { out = FrameGenId::XessFg;         return true; }
    if (s == "CSR-FG")  { out = FrameGenId::CsrInterpolated; return true; }
    return false;
}

}
