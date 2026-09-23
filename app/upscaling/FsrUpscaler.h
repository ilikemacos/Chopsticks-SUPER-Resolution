#pragma once

#include "IUpscaler.h"

namespace ufx {

// FSR 1/2/3/4 share this adapter; the version distinguishes which
// requirements and quality modes are exposed.
class FsrUpscaler : public IUpscaler {
public:
    explicit FsrUpscaler(int majorVersion);
    UpscalerCaps Caps() const override;
    Availability CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const override;
    std::optional<std::pair<uint32_t, uint32_t>>
        RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const override;

private:
    int version_;
};

}
