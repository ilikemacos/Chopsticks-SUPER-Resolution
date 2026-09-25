#pragma once

// Image file I/O for the CSR command-line upscaler.
//
// Loads common formats (PNG, JPEG, BMP, TGA, ...) and writes PNG, converting to
// and from the tightly packed float RGB buffer CSR operates on. Backed by stb,
// which is public-domain single-header code — not a proprietary vendor binary,
// so bundling it is consistent with CONTRIBUTING.md.
//
// CSR expects perceptually-encoded (sRGB/gamma) 8-bit-derived values, which is
// exactly what an ordinary image file holds, so no colour conversion is applied.

#include "CsrCore.h"

#include <string>

namespace ufx::csr {

struct LoadResult {
    bool ok = false;
    std::string error;   // empty on success
    Image image;
};

// Decodes an image file to a float RGB buffer in [0,1]. Alpha, if present, is
// dropped (CSR is an RGB spatial filter). Never throws.
LoadResult LoadImage(const std::string& path);

// Encodes a float RGB buffer to a PNG at 8 bits per channel. Values are rounded
// to nearest and clamped; NaN maps to 0. Returns false with a reason on failure.
bool SavePng(const Image& img, const std::string& path, std::string& error);

}
