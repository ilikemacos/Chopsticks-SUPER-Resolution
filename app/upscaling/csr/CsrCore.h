#pragma once

// CSR — Chopsticks Super Resolution. The actual upscaling maths.
//
// This is a port of csr/ref/csr.py, which is the specification, and is kept in
// step with the C# port in csr/src/Csr.Core. All three are verified against
// csr/ref/golden/*.json to within one 8-bit step; tests/test_csr.cpp is the
// gate for this one.
//
// Deliberately free of any Windows or Direct3D dependency so it compiles and is
// tested on CI hosts that are not Windows, exactly like the rest of app/core.

#include <cstdint>
#include <vector>

namespace ufx::csr {

// Tightly packed float RGB image, row-major, three floats per pixel.
//
// Values are expected to be perceptually encoded (sRGB/gamma) in [0,1], not
// linear light. The edge detection assumes this; feeding it linear values
// produces haloing that looks like an algorithm fault but is not one.
struct Image {
    int width = 0;
    int height = 0;
    std::vector<float> data;   // width * height * 3

    Image() = default;
    Image(int w, int h) : width(w), height(h), data(static_cast<size_t>(w) * h * 3, 0.0f) {}

    // Index of the first component of a pixel, with edge clamping.
    [[nodiscard]] size_t ClampedIndex(int x, int y) const {
        if (x < 0) x = 0; else if (x >= width) x = width - 1;
        if (y < 0) y = 0; else if (y >= height) y = height - 1;
        return (static_cast<size_t>(y) * width + x) * 3;
    }
};

struct Options {
    // Sharpening strength in stops; higher is weaker. 0.25 is the validated default.
    float sharpnessStops = 0.25f;
    // Back off sharpening where there is little local detail.
    bool adaptiveSharpen = true;
    // Clamp the resolve to the four nearest taps. This is what prevents
    // overshoot; it is the step naive ports drop.
    bool dering = true;
};

// Edge-directed resolve pass (16 taps, Rec.709 luma direction estimate).
Image Resolve(const Image& src, int outWidth, int outHeight, const Options& o = {});

// Contrast-limited sharpening at output resolution. Cannot clip, by construction.
Image Sharpen(const Image& img, const Options& o = {});

// The full pipeline: resolve then sharpen.
//
// At 1:1 this returns a copy without touching the pixels. The resolve is a
// filter with a negative lobe, not a pass-through, so running it at 1.0x would
// visibly alter a frame the user asked not to be upscaled.
Image Upscale(const Image& src, int outWidth, int outHeight, const Options& o = {});

// Centre-aligned bilinear resample — the baseline CSR has to beat. Kept here so
// a comparison is against a named, pinned filter rather than whatever the host
// image stack happens to do.
Image Bilinear(const Image& src, int outWidth, int outHeight);

}

namespace ufx::csr {
// CSR 1.0 is proprietary; see csr/LICENSE. Derived from AMD FSR 1 (MIT,
// attribution retained in csr/NOTICE.md).
inline constexpr const char* kVersion = "1.0.0";
inline constexpr const char* kVersionDisplay = "1.0";
}
