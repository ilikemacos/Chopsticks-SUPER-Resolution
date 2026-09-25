#pragma once

// ufx-live: a measured, real-time CSR upscale loop.
//
// The shipping real-time path is the GPU compute shader in csr/src/Csr.Capture
// (Windows Graphics Capture -> CSR compute -> DXGI present). That path cannot be
// executed on a machine without a GPU, so this is its cross-platform CPU
// counterpart: it runs the *same* verified CSR core (csr/CsrCore) in the same
// capture -> upscale -> present control flow, over a stream of synthetic frames,
// and reports the sustained per-frame cost and throughput.
//
// It is deliberately honest about what it is. The number it prints is the CPU
// reference cost, not the GPU real-time cost; the GPU path is far cheaper. What
// this proves is that the frame loop works end to end and that CSR produces a
// correctly sized, upscaled frame every tick.

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "upscaling/csr/CsrCore.h"

namespace ufx {

struct LiveArgs {
    // The synthetic "captured" source resolution.
    uint32_t srcWidth = 1280;
    uint32_t srcHeight = 720;
    // Output size: from the quality preset unless width/height are both set.
    std::string quality = "Quality";   // a QualityMode name
    uint32_t outWidth = 0;
    uint32_t outHeight = 0;
    // How many frames to push through the loop.
    uint32_t frames = 120;
    float sharpnessStops = 0.25f;
    bool dering = true;
    bool adaptiveSharpen = true;
    // Optional: write the final upscaled frame here, as proof the loop produced
    // real pixels rather than just a timer reading.
    std::string saveLastPath;
    bool quiet = false;
};

struct LiveStats {
    uint32_t framesProcessed = 0;
    uint32_t srcW = 0, srcH = 0, outW = 0, outH = 0;
    double totalUpscaleMs = 0.0;   // summed per-frame upscale time
    double minUpscaleMs = 0.0;
    double maxUpscaleMs = 0.0;
    double wallMs = 0.0;           // end-to-end wall time of the whole loop

    [[nodiscard]] double MeanUpscaleMs() const {
        return framesProcessed ? totalUpscaleMs / framesProcessed : 0.0;
    }
    // Throughput the upscale step alone could sustain.
    [[nodiscard]] double UpscaleFps() const {
        return totalUpscaleMs > 0.0 ? framesProcessed * 1000.0 / totalUpscaleMs : 0.0;
    }
    // Throughput the whole loop sustained (generate + upscale + sink).
    [[nodiscard]] double WallFps() const {
        return wallMs > 0.0 ? framesProcessed * 1000.0 / wallMs : 0.0;
    }
};

// Parses argv (excluding the program name). Returns true and fills `args`, or
// false and writes a message to `err`. `showHelp` is set when help was asked for.
bool ParseLiveArgs(const std::vector<std::string>& argv, LiveArgs& args,
                   std::string& err, bool& showHelp);

void PrintLiveUsage(std::ostream& out);

// Produces one synthetic source frame at time index `t`. The scene has moving
// hard edges and moving mid-contrast detail, so CSR's resolve and adaptive
// sharpen both do real work every frame, the way they would on live content.
csr::Image MakeLiveFrame(int width, int height, uint32_t t);

// The measured loop, exposed for tests. Generates `args.frames` frames, upscales
// each with CSR, hands every upscaled frame to `sink` (which may be empty), and
// returns timing. This is exactly the body the GPU path runs per FrameArrived,
// minus the GPU.
LiveStats RunLiveLoop(const LiveArgs& args,
                      const std::function<void(uint32_t, const csr::Image&)>& sink);

// Runs the loop and prints a report. Returns a process exit code (0 == success).
int RunLive(const LiveArgs& args, std::ostream& out, std::ostream& err);

}  // namespace ufx
