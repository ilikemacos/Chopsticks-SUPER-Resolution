#pragma once

#include "IUpscaler.h"

namespace ufx {

class XessUpscaler : public IUpscaler {
public:
    UpscalerCaps Caps() const override;
    Availability CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const override;
    std::optional<std::pair<uint32_t, uint32_t>>
        RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const override;
};

}
