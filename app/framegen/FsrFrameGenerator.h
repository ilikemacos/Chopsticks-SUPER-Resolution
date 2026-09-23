#pragma once

#include "IFrameGenerator.h"

namespace ufx {

// Adapter describing AMD FSR 3 Frame Generation. It does not perform frame
// generation itself; it reports honest availability and requirements so the UI
// can configure a game that already integrates FSR 3 FG.
class FsrFrameGenerator : public IFrameGenerator {
public:
    FrameGenCaps Caps() const override;
    FrameGenState Evaluate(const GpuInfo& gpu, GraphicsApi api,
                           bool gameDeclaresSupport) const override;
};

// Intel XeSS Frame Generation adapter.
class XessFrameGenerator : public IFrameGenerator {
public:
    FrameGenCaps Caps() const override;
    FrameGenState Evaluate(const GpuInfo& gpu, GraphicsApi api,
                           bool gameDeclaresSupport) const override;
};

}
