#pragma once

#include "upscaling/IUpscaler.h"
#include "gpu/GpuInfo.h"

#include <string>
#include <vector>

namespace ufx {

enum class FrameGenId {
    None,
    Fsr3Fg,          // AMD FidelityFX Frame Generation (part of FSR 3)
    XessFg,          // Intel XeSS Frame Generation
    CsrInterpolated, // Our own external optical-flow interpolation (any app)
};

// Motion-vector frame generation (FSR 3 FG / XeSS FG / DLSS 3) is fundamentally a
// *game-integrated* technology: it needs the swapchain, motion vectors and UI
// composition, so it cannot be added to an arbitrary game from outside.
//
// The one exception is CsrInterpolated: optical-flow interpolation between two
// frames the app already presented. It needs no engine data, so it works on any
// app -- but it ADDS latency and smears on disocclusion/HUD, so it is smoothness,
// never free performance. Measured in csr/FRAMEGEN.md.
enum class FrameGenState {
    Disabled,
    Supported,                // Available and configurable for this game
    RequiresGameIntegration,  // The game must ship this technology
    RequiresCompatibleImpl,   // Needs a specific runtime/driver not present
};

struct FrameGenCaps {
    FrameGenId id = FrameGenId::None;
    std::string displayName;
    std::string vendorName;
    std::vector<GraphicsApi> supportedApis;
    std::string requirements;
    // Honest note shown verbatim in the UI.
    std::string externalLimitation;
};

class IFrameGenerator {
public:
    virtual ~IFrameGenerator() = default;
    virtual FrameGenCaps Caps() const = 0;
    // Given the GPU/API and whether the *game* declares frame-gen support in its
    // profile, return the state UFX should display. UFX never claims support the
    // game does not have.
    virtual FrameGenState Evaluate(const GpuInfo& gpu, GraphicsApi api,
                                   bool gameDeclaresSupport) const = 0;
};

const char* FrameGenStateLabel(FrameGenState s);
const char* FrameGenIdName(FrameGenId id);
bool ParseFrameGenId(std::string_view s, FrameGenId& out);

}
