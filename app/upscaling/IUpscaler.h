#pragma once

#include "QualityMode.h"
#include "gpu/GpuInfo.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ufx {

enum class GraphicsApi {
    D3D11,
    D3D12,
    Vulkan,
};

enum class UpscalerId {
    None,
    Fsr1,
    Fsr2,
    Fsr3,
    Fsr4,
    XeSS,
};

// Why an upscaler may be unavailable on the current system.
struct Availability {
    bool available = false;
    std::string reason;   // empty when available == true

    static Availability Yes() { return {true, {}}; }
    static Availability No(std::string why) { return {false, std::move(why)}; }
};

struct UpscalerCaps {
    UpscalerId id = UpscalerId::None;
    std::string displayName;
    std::string vendorName;
    std::vector<GraphicsApi> supportedApis;
    std::vector<QualityMode> supportedModes;
    bool sharpnessSupported = false;
    // Human-readable one-liner: "Motion vectors, depth and jitter from the game."
    std::string requirements;
    // Where the technology can only be brought in by the game engine itself.
    bool gameIntegrationRequired = true;
    // True if an external DLL-swap path exists for at least some games.
    bool externalWrapperExists = false;
};

// An upscaler adapter as far as UFX is concerned is a **descriptor + availability probe**.
// UFX does not run the upscaler itself against a live game process; it only configures it.
class IUpscaler {
public:
    virtual ~IUpscaler() = default;

    virtual UpscalerCaps Caps() const = 0;
    virtual Availability CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const = 0;

    // Compute the internal render size the game engine should target for a given
    // output resolution and quality mode. Returns nullopt when the mode is not supported.
    virtual std::optional<std::pair<uint32_t, uint32_t>>
        RenderResolutionFor(uint32_t outWidth, uint32_t outHeight, QualityMode mode) const = 0;
};

}
