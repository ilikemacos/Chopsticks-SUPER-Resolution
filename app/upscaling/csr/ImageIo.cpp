#include "ImageIo.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace ufx::csr {

LoadResult LoadImage(const std::string& path) {
    LoadResult r;
    int w = 0, h = 0, channels = 0;
    // Force 3 channels: CSR is RGB. stbi handles grayscale/RGBA expansion.
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 3);
    if (!data) {
        r.error = std::string("could not read '") + path + "': " + stbi_failure_reason();
        return r;
    }
    if (w <= 0 || h <= 0) {
        stbi_image_free(data);
        r.error = "image has non-positive dimensions";
        return r;
    }

    r.image = Image(w, h);
    const size_t n = static_cast<size_t>(w) * h * 3;
    for (size_t i = 0; i < n; ++i) r.image.data[i] = data[i] / 255.0f;
    stbi_image_free(data);
    r.ok = true;
    return r;
}

namespace {
inline unsigned char Quantise(float v) {
    if (std::isnan(v)) return 0;
    float s = v * 255.0f + 0.5f;
    if (s < 0.0f) s = 0.0f; else if (s > 255.0f) s = 255.0f;
    return static_cast<unsigned char>(s);
}
}  // namespace

bool SavePng(const Image& img, const std::string& path, std::string& error) {
    if (img.width <= 0 || img.height <= 0) {
        error = "refusing to write an image with non-positive dimensions";
        return false;
    }
    std::vector<unsigned char> bytes(static_cast<size_t>(img.width) * img.height * 3);
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = Quantise(img.data[i]);

    const int stride = img.width * 3;
    if (stbi_write_png(path.c_str(), img.width, img.height, 3, bytes.data(), stride) == 0) {
        error = std::string("could not write '") + path + "'";
        return false;
    }
    error.clear();
    return true;
}

}
