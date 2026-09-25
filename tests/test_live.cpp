// Proves the CSR real-time loop works end to end on the CPU reference path:
// every tick a synthetic frame is captured, upscaled by the same CSR core the
// GPU path dispatches, and delivered to a sink at the correct output size.
//
// This does not measure the GPU. It guards the loop's control flow and the
// contract that each frame out is (a) the right size, (b) valid pixels, and
// (c) actually different frame to frame (a live source, not a frozen image).

#include "cli/LiveCommand.h"
#include "upscaling/csr/CsrCore.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using namespace ufx;

namespace {

bool AllFiniteInRange(const csr::Image& img) {
    for (float c : img.data) {
        if (!std::isfinite(c) || c < 0.0f || c > 1.0f) return false;
    }
    return true;
}

double MeanAbsDiff(const csr::Image& a, const csr::Image& b) {
    if (a.data.size() != b.data.size() || a.data.empty()) return 0.0;
    double s = 0.0;
    for (size_t i = 0; i < a.data.size(); ++i) s += std::abs(a.data[i] - b.data[i]);
    return s / a.data.size();
}

}  // namespace

TEST(Live, ParsesSourceAndQuality) {
    LiveArgs args;
    std::string err;
    bool help = false;
    ASSERT_TRUE(ParseLiveArgs({"--src", "640x480", "--quality", "Performance",
                               "--frames", "3"}, args, err, help))
        << err;
    EXPECT_FALSE(help);
    EXPECT_EQ(args.srcWidth, 640u);
    EXPECT_EQ(args.srcHeight, 480u);
    EXPECT_EQ(args.quality, "Performance");
    EXPECT_EQ(args.frames, 3u);
}

TEST(Live, RejectsBadSourceAndQuality) {
    LiveArgs args;
    std::string err;
    bool help = false;
    EXPECT_FALSE(ParseLiveArgs({"--src", "not-a-size"}, args, err, help));
    EXPECT_FALSE(ParseLiveArgs({"--quality", "Nonsense"}, args, err, help));
    EXPECT_FALSE(ParseLiveArgs({"--frames", "0"}, args, err, help));
}

TEST(Live, SyntheticFrameChangesOverTime) {
    // A live source must not be frozen: consecutive frames must differ, or the
    // loop would be a still-image benchmark wearing a live costume.
    const csr::Image f0 = MakeLiveFrame(96, 64, 0);
    const csr::Image f1 = MakeLiveFrame(96, 64, 1);
    const csr::Image f9 = MakeLiveFrame(96, 64, 9);
    EXPECT_GT(MeanAbsDiff(f0, f1), 0.0);
    EXPECT_GT(MeanAbsDiff(f0, f9), MeanAbsDiff(f0, f1));
    EXPECT_TRUE(AllFiniteInRange(f0));
}

TEST(Live, LoopUpscalesEveryFrameToTargetSize) {
    LiveArgs args;
    args.srcWidth = 128;
    args.srcHeight = 72;
    args.quality = "Quality";   // 1.5x -> 192x108
    args.frames = 5;

    std::vector<std::pair<int, int>> sizes;
    int calls = 0;
    csr::Image firstOut, lastOut;
    const LiveStats s = RunLiveLoop(args, [&](uint32_t t, const csr::Image& up) {
        sizes.emplace_back(up.width, up.height);
        if (t == 0) firstOut = up;
        lastOut = up;
        EXPECT_TRUE(AllFiniteInRange(up));
        ++calls;
    });

    EXPECT_EQ(s.framesProcessed, 5u);
    EXPECT_EQ(calls, 5);
    EXPECT_EQ(s.outW, 192u);
    EXPECT_EQ(s.outH, 108u);
    for (auto [w, h] : sizes) {
        EXPECT_EQ(w, 192);
        EXPECT_EQ(h, 108);
    }
    // The upscaled output must also move frame to frame.
    EXPECT_GT(MeanAbsDiff(firstOut, lastOut), 0.0);
}

TEST(Live, ExplicitSizeOverridesQuality) {
    LiveArgs args;
    args.srcWidth = 100;
    args.srcHeight = 100;
    args.outWidth = 250;
    args.outHeight = 175;
    args.frames = 2;
    const LiveStats s = RunLiveLoop(args, nullptr);
    EXPECT_EQ(s.outW, 250u);
    EXPECT_EQ(s.outH, 175u);
    EXPECT_EQ(s.framesProcessed, 2u);
}

TEST(Live, ReportsPositiveThroughput) {
    LiveArgs args;
    args.srcWidth = 160;
    args.srcHeight = 90;
    args.frames = 8;
    const LiveStats s = RunLiveLoop(args, nullptr);
    EXPECT_EQ(s.framesProcessed, 8u);
    EXPECT_GT(s.totalUpscaleMs, 0.0);
    EXPECT_GT(s.UpscaleFps(), 0.0);
    EXPECT_GT(s.WallFps(), 0.0);
    EXPECT_LE(s.minUpscaleMs, s.MeanUpscaleMs() + 1e-9);
    EXPECT_GE(s.maxUpscaleMs, s.MeanUpscaleMs() - 1e-9);
}
