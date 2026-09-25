#include "CsrCore.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <thread>
#include <vector>

namespace ufx::csr {
namespace {

// Runs `body(y)` for every row in [0, height) across the hardware's cores, each
// thread owning a disjoint band of rows. Output is therefore identical to the
// serial order regardless of thread count, so the golden vectors still hold.
// Small images stay single-threaded, where thread setup would cost more than it
// saves. CSR_THREADS caps the count (0/unset = auto); 1 forces serial.
template <typename Body>
void ParallelForRows(int height, int width, Body&& body) {
    unsigned hw = std::thread::hardware_concurrency();
    if (const char* env = std::getenv("CSR_THREADS")) {
        char* end = nullptr;
        unsigned long v = std::strtoul(env, &end, 10);
        if (end != env) hw = static_cast<unsigned>(v);
    }
    if (hw == 0) hw = 1;

    // Below this many output pixels the overhead is not worth it.
    const long long work = static_cast<long long>(height) * width;
    unsigned threads = (work < 64 * 1024 || height < 2) ? 1u
                       : std::min<unsigned>(hw, static_cast<unsigned>(height));

    if (threads <= 1) {
        for (int y = 0; y < height; ++y) body(y);
        return;
    }

    std::vector<std::thread> pool;
    pool.reserve(threads - 1);
    const int band = (height + static_cast<int>(threads) - 1) / static_cast<int>(threads);
    for (unsigned t = 1; t < threads; ++t) {
        const int y0 = static_cast<int>(t) * band;
        const int y1 = std::min(height, y0 + band);
        if (y0 >= y1) break;
        pool.emplace_back([&body, y0, y1] { for (int y = y0; y < y1; ++y) body(y); });
    }
    for (int y = 0; y < std::min(height, band); ++y) body(y);  // this thread takes band 0
    for (auto& th : pool) th.join();
}

// Rec.709 luma. Measured +0.22 dB over FSR 1's cheap 0.5B+0.5R+G proxy on
// content where the channels disagree.
constexpr float kLumaR = 0.2126f;
constexpr float kLumaG = 0.7152f;
constexpr float kLumaB = 0.0722f;

// Window of the sharpening negative lobe, from FSR 1.
constexpr float kRcasLimit = 0.25f - (1.0f / 16.0f);

inline float SafeDiv(float n, float d) { return d != 0.0f ? n / d : 0.0f; }
inline float Sat(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline float Luma(const std::vector<float>& p, size_t i) {
    return kLumaR * p[i] + kLumaG * p[i + 1] + kLumaB * p[i + 2];
}

// Accumulates edge direction and strength for one quadrant of the '+' pattern
// around a centre tap. FSR 1's estimator, kept because the structure-tensor
// alternative measured 0.38 dB worse.
inline void Quadrant(float lA, float lB, float lC, float lD, float lE,
                     float w, float& dirX, float& dirY, float& len) {
    const float dc = lD - lC, cb = lC - lB;
    const float mx = std::max(std::fabs(dc), std::fabs(cb));
    const float gx = lD - lB;
    dirX += gx * w;
    const float lx = Sat(SafeDiv(std::fabs(gx), mx));
    len += lx * lx * w;

    const float ec = lE - lC, ca = lC - lA;
    const float my = std::max(std::fabs(ec), std::fabs(ca));
    const float gy = lE - lA;
    dirY += gy * w;
    const float ly = Sat(SafeDiv(std::fabs(gy), my));
    len += ly * ly * w;
}

// Separable 3x3 box blur with edge clamping.
std::vector<float> Box3(const std::vector<float>& src, int w, int h) {
    std::vector<float> tmp(static_cast<size_t>(w) * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int xm = x > 0 ? x - 1 : 0;
            const int xp = x < w - 1 ? x + 1 : w - 1;
            const size_t row = static_cast<size_t>(y) * w;
            tmp[row + x] = (src[row + xm] + src[row + x] + src[row + xp]) / 3.0f;
        }
    }
    std::vector<float> out(static_cast<size_t>(w) * h);
    for (int y = 0; y < h; ++y) {
        const int ym = y > 0 ? y - 1 : 0;
        const int yp = y < h - 1 ? y + 1 : h - 1;
        for (int x = 0; x < w; ++x) {
            out[static_cast<size_t>(y) * w + x] =
                (tmp[static_cast<size_t>(ym) * w + x] + tmp[static_cast<size_t>(y) * w + x] +
                 tmp[static_cast<size_t>(yp) * w + x]) / 3.0f;
        }
    }
    return out;
}

}  // namespace

Image Resolve(const Image& src, int outWidth, int outHeight, const Options& o) {
    Image dst(outWidth, outHeight);
    const std::vector<float>& s = src.data;
    std::vector<float>& d = dst.data;

    const float scaleX = static_cast<float>(src.width) / outWidth;
    const float scaleY = static_cast<float>(src.height) / outHeight;

    // Full 4x4 support. FSR 1 drops the four corners; including them is worth
    // +0.53 dB because they carry the diagonal-edge information.
    int offX[16], offY[16];
    int n = 0;
    for (int dy = -1; dy <= 2; ++dy)
        for (int dx = -1; dx <= 2; ++dx) { offX[n] = dx; offY[n] = dy; ++n; }

    ParallelForRows(outHeight, outWidth, [&](int oy) {
        const float ppY = (oy + 0.5f) * scaleY - 0.5f;
        const int ipY = static_cast<int>(std::floor(ppY));
        const float fy = ppY - ipY;

        for (int ox = 0; ox < outWidth; ++ox) {
            const float ppX = (ox + 0.5f) * scaleX - 0.5f;
            const int ipX = static_cast<int>(std::floor(ppX));
            const float fx = ppX - ipX;

            // Luma of the 12 taps the estimator needs.
            const float lb = Luma(s, src.ClampedIndex(ipX,     ipY - 1));
            const float lc = Luma(s, src.ClampedIndex(ipX + 1, ipY - 1));
            const float le = Luma(s, src.ClampedIndex(ipX - 1, ipY));
            const float lf = Luma(s, src.ClampedIndex(ipX,     ipY));
            const float lg = Luma(s, src.ClampedIndex(ipX + 1, ipY));
            const float lh = Luma(s, src.ClampedIndex(ipX + 2, ipY));
            const float li = Luma(s, src.ClampedIndex(ipX - 1, ipY + 1));
            const float lj = Luma(s, src.ClampedIndex(ipX,     ipY + 1));
            const float lk = Luma(s, src.ClampedIndex(ipX + 1, ipY + 1));
            const float ll = Luma(s, src.ClampedIndex(ipX + 2, ipY + 1));
            const float ln = Luma(s, src.ClampedIndex(ipX,     ipY + 2));
            const float lo = Luma(s, src.ClampedIndex(ipX + 1, ipY + 2));

            const float wF = (1.0f - fx) * (1.0f - fy);
            const float wG = fx * (1.0f - fy);
            const float wJ = (1.0f - fx) * fy;
            const float wK = fx * fy;

            float dirX = 0.0f, dirY = 0.0f, len = 0.0f;
            Quadrant(lb, le, lf, lg, lj, wF, dirX, dirY, len);
            Quadrant(lc, lf, lg, lh, lk, wG, dirX, dirY, len);
            Quadrant(lf, li, lj, lk, ln, wJ, dirX, dirY, len);
            Quadrant(lg, lj, lk, ll, lo, wK, dirX, dirY, len);

            // Normalise direction; a near-zero gradient means "no edge".
            const float dir2 = dirX * dirX + dirY * dirY;
            const bool zero = dir2 < (1.0f / 32768.0f);
            const float inv = zero ? 1.0f : SafeDiv(1.0f, std::sqrt(dir2));
            dirX = (zero ? 1.0f : dirX) * inv;
            dirY = (zero ? 0.0f : dirY) * inv;

            len *= 0.5f;
            len *= len;

            // Anisotropy: narrow across the edge, widen along it.
            const float m = std::max(std::fabs(dirX), std::fabs(dirY));
            const float stretch = SafeDiv(dirX * dirX + dirY * dirY, m);
            const float lenAcross = 1.0f + (stretch - 1.0f) * len;
            const float lenAlong = 1.0f - 0.5f * len;

            const float lob = 0.5f + ((1.0f / 4.0f - 0.04f) - 0.5f) * len;
            const float clp = SafeDiv(1.0f, lob);

            float accR = 0.0f, accG = 0.0f, accB = 0.0f, accW = 0.0f;
            for (int t = 0; t < 16; ++t) {
                const size_t idx = src.ClampedIndex(ipX + offX[t], ipY + offY[t]);
                const float offx = offX[t] - fx;
                const float offy = offY[t] - fy;

                const float vx = (offx * dirX + offy * dirY) * lenAcross;
                const float vy = (offx * -dirY + offy * dirX) * lenAlong;
                const float d2 = std::min(vx * vx + vy * vy, clp);

                // FSR 1's polynomial lobe. Its radius is coupled to edge
                // strength, which a fixed-support cubic cannot reproduce — the
                // Keys alternative measured 2.13 dB worse.
                float wB = (2.0f / 5.0f) * d2 - 1.0f;
                float wA = lob * d2 - 1.0f;
                wB *= wB;
                wA *= wA;
                wB = (25.0f / 16.0f) * wB - (25.0f / 16.0f - 1.0f);
                const float w = wB * wA;

                accR += s[idx] * w;
                accG += s[idx + 1] * w;
                accB += s[idx + 2] * w;
                accW += w;
            }

            float rr = SafeDiv(accR, accW);
            float gg = SafeDiv(accG, accW);
            float bb = SafeDiv(accB, accW);

            if (o.dering) {
                const size_t i0 = src.ClampedIndex(ipX,     ipY);
                const size_t i1 = src.ClampedIndex(ipX + 1, ipY);
                const size_t i2 = src.ClampedIndex(ipX,     ipY + 1);
                const size_t i3 = src.ClampedIndex(ipX + 1, ipY + 1);
                for (int c = 0; c < 3; ++c) {
                    const float a = s[i0 + c], b2 = s[i1 + c], c2 = s[i2 + c], e2 = s[i3 + c];
                    const float lo2 = std::min(std::min(a, b2), std::min(c2, e2));
                    const float hi2 = std::max(std::max(a, b2), std::max(c2, e2));
                    float v = c == 0 ? rr : (c == 1 ? gg : bb);
                    v = std::min(hi2, std::max(lo2, v));
                    if (c == 0) rr = v; else if (c == 1) gg = v; else bb = v;
                }
            }

            const size_t out = (static_cast<size_t>(oy) * outWidth + ox) * 3;
            d[out]     = Sat(rr);
            d[out + 1] = Sat(gg);
            d[out + 2] = Sat(bb);
        }
    });

    return dst;
}

Image Sharpen(const Image& img, const Options& o) {
    const int w = img.width, h = img.height;
    Image dst(w, h);
    const std::vector<float>& s = img.data;
    std::vector<float>& d = dst.data;

    const float sharp = std::pow(2.0f, -o.sharpnessStops);

    // Local mean and mean-of-squares of luma, for the adaptive term.
    std::vector<float> mean, meanSq;
    if (o.adaptiveSharpen) {
        std::vector<float> lum(static_cast<size_t>(w) * h);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                lum[static_cast<size_t>(y) * w + x] = Luma(s, img.ClampedIndex(x, y));
        mean = Box3(lum, w, h);
        std::vector<float> sq(lum.size());
        for (size_t i = 0; i < sq.size(); ++i) sq[i] = lum[i] * lum[i];
        meanSq = Box3(sq, w, h);
    }

    ParallelForRows(h, w, [&](int y) {
        for (int x = 0; x < w; ++x) {
            const size_t ie = img.ClampedIndex(x,     y);
            const size_t ib = img.ClampedIndex(x,     y - 1);
            const size_t id = img.ClampedIndex(x - 1, y);
            const size_t ir = img.ClampedIndex(x + 1, y);
            const size_t ih = img.ClampedIndex(x,     y + 1);

            // Per-channel limiter: how much negative lobe fits before clipping.
            float lobe = -std::numeric_limits<float>::infinity();
            for (int c = 0; c < 3; ++c) {
                const float b = s[ib + c], dd = s[id + c], ff = s[ir + c], hh = s[ih + c];
                const float mn = std::min(std::min(b, dd), std::min(ff, hh));
                const float mx = std::max(std::max(b, dd), std::max(ff, hh));
                const float hitMin = SafeDiv(mn, 4.0f * mx);
                const float hitMax = SafeDiv(1.0f - mx, 4.0f * mn - 4.0f);
                const float l = std::max(-hitMin, hitMax);
                if (l > lobe) lobe = l;
            }
            lobe = std::max(-kRcasLimit, std::min(lobe, 0.0f)) * sharp;

            if (o.adaptiveSharpen) {
                // Back off where there is little local detail; sharpening there
                // only lifts noise. Worth +2.57 dB on mid-contrast content, and
                // exactly 0 where the limiter already refuses.
                const size_t p = static_cast<size_t>(y) * w + x;
                const float var = meanSq[p] - mean[p] * mean[p];
                lobe *= Sat(var * 400.0f);
            }

            const size_t out = (static_cast<size_t>(y) * w + x) * 3;
            const float denom = 4.0f * lobe + 1.0f;
            for (int c = 0; c < 3; ++c) {
                const float acc = lobe * s[ib + c] + lobe * s[id + c]
                                + lobe * s[ir + c] + lobe * s[ih + c] + s[ie + c];
                d[out + c] = Sat(SafeDiv(acc, denom));
            }
        }
    });

    return dst;
}

Image Upscale(const Image& src, int outWidth, int outHeight, const Options& o) {
    if (outWidth == src.width && outHeight == src.height) return src;
    return Sharpen(Resolve(src, outWidth, outHeight, o), o);
}

Image Bilinear(const Image& src, int outWidth, int outHeight) {
    Image dst(outWidth, outHeight);
    const float sx = static_cast<float>(src.width) / outWidth;
    const float sy = static_cast<float>(src.height) / outHeight;

    for (int y = 0; y < outHeight; ++y) {
        const float py = (y + 0.5f) * sy - 0.5f;
        const int y0 = static_cast<int>(std::floor(py));
        const float ty = py - y0;

        for (int x = 0; x < outWidth; ++x) {
            const float px = (x + 0.5f) * sx - 0.5f;
            const int x0 = static_cast<int>(std::floor(px));
            const float tx = px - x0;

            const size_t i00 = src.ClampedIndex(x0,     y0);
            const size_t i10 = src.ClampedIndex(x0 + 1, y0);
            const size_t i01 = src.ClampedIndex(x0,     y0 + 1);
            const size_t i11 = src.ClampedIndex(x0 + 1, y0 + 1);

            const size_t out = (static_cast<size_t>(y) * outWidth + x) * 3;
            for (int c = 0; c < 3; ++c) {
                const float top = src.data[i00 + c] * (1.0f - tx) + src.data[i10 + c] * tx;
                const float bot = src.data[i01 + c] * (1.0f - tx) + src.data[i11 + c] * tx;
                dst.data[out + c] = top * (1.0f - ty) + bot * ty;
            }
        }
    }
    return dst;
}

}
