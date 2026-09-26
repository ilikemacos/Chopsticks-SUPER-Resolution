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

// CSR frame generation: our own external optical-flow interpolation. Unlike the
// two above it needs no game integration -- it runs on the frames any app
// presents (via the capture path) -- so it reports Supported regardless of the
// game, while its externalLimitation states the latency/artifact costs honestly.
class CsrFrameGenerator : public IFrameGenerator {
public:
    FrameGenCaps Caps() const override;
    FrameGenState Evaluate(const GpuInfo& gpu, GraphicsApi api,
                           bool gameDeclaresSupport) const override;
};

}
