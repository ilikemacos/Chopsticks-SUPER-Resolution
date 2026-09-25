#pragma once

#include "IUpscaler.h"
#include "csr/CsrCore.h"

namespace ufx {

// CSR — Chopsticks Super Resolution. Our own spatial upscaler, derived from
// FSR 1's EASU + RCAS design.
//
// This is the only entry in the registry the application can actually execute
// rather than merely configure: the maths lives in csr/CsrCore.cpp and is
// compiled into the binary. Every other adapter is a descriptor for something
// the game itself must ship.
class CsrUpscaler : public IUpscaler {
public:
    UpscalerCaps Caps() const override;
    Availability CheckAvailability(const GpuInfo& gpu, GraphicsApi api) const override;
    std::optional<std::pair<uint32_t, uint32_t>>
        RenderResolutionFor(uint32_t w, uint32_t h, QualityMode m) const override;

    // Runs CSR for real. Present so callers do not have to reach past the
    // registry into csr::Upscale, and so "this upscaler can be run" is a
    // property of the adapter rather than a claim made elsewhere.
    static csr::Image Run(const csr::Image& src, uint32_t outWidth, uint32_t outHeight,
                          const csr::Options& options = {});
};

}
