// Proves the CSR file upscaler actually upscales: it writes a real image, runs
// the same RunUpscale() the shipped tool and GUI use, reads the result back, and
// checks it is (a) the right size and (b) genuinely CSR output — measurably
// closer to a high-res ground truth than a plain bilinear resize.

#include "cli/UpscaleCommand.h"
#include "upscaling/csr/CsrCore.h"
#include "upscaling/csr/ImageIo.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <filesystem>
#include <random>
#include <sstream>

namespace fs = std::filesystem;
using namespace ufx;

namespace {

fs::path TempDir() {
    static std::atomic<unsigned> counter{0};
    std::random_device rd;
    for (int attempt = 0; attempt < 64; ++attempt) {
        auto p = fs::temp_directory_path() /
                 ("ufx_cli_" + std::to_string(rd()) + "_" + std::to_string(counter++));
        std::error_code ec;
        if (fs::create_directory(p, ec) && !ec) return p;
    }
    throw std::runtime_error("could not create temp dir");
}

// A deterministic diagonal-edge scene, area-averaged (supersampled) so its edges
// are anti-aliased exactly as a real photographed or rendered image's are. This
// matters: a hard per-pixel threshold would carry no sub-pixel edge information,
// and then there is nothing for an edge-aware upscaler to reconstruct that a
// blur cannot fake. Sampling the same continuous scene at two resolutions is how
// csr/ref/validate.py measures CSR's advantage, and this mirrors it.
csr::Image Scene(int w, int h) {
    constexpr int kSS = 4;   // 4x4 supersampling per output pixel
    csr::Image img(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            float cover = 0.0f;
            for (int sy = 0; sy < kSS; ++sy)
                for (int sx = 0; sx < kSS; ++sx) {
                    const float u = (x + (sx + 0.5f) / kSS) / w;
                    const float v = (y + (sy + 0.5f) / kSS) / h;
                    if (u + v > 1.0f) cover += 1.0f;
                }
            cover /= (kSS * kSS);                 // fractional edge coverage in [0,1]
            const float edge = 0.15f + 0.70f * cover;
            const size_t i = img.ClampedIndex(x, y);
            img.data[i]     = edge;
            img.data[i + 1] = edge * 0.9f + 0.05f;
            img.data[i + 2] = 1.0f - edge;
        }
    return img;
}

double Psnr(const csr::Image& a, const csr::Image& b) {
    EXPECT_EQ(a.data.size(), b.data.size());
    double mse = 0.0;
    for (size_t i = 0; i < a.data.size(); ++i) {
        const double d = a.data[i] - b.data[i];
        mse += d * d;
    }
    mse /= a.data.size();
    if (mse <= 1e-12) return 99.0;
    return 10.0 * std::log10(1.0 / mse);
}

}  // namespace

TEST(UpscaleCli, WritesARealUpscaledImageThatBeatsBilinear) {
    const fs::path dir = TempDir();
    const fs::path in = dir / "in.png";
    const fs::path out = dir / "out.png";

    // Ground truth at 2x, a low-res input at 1x, and the bilinear baseline we
    // must beat, all from the same scene.
    const csr::Image truth = Scene(160, 120);
    const csr::Image low = Scene(80, 60);
    std::string werr;
    ASSERT_TRUE(csr::SavePng(low, in.string(), werr)) << werr;

    UpscaleArgs args;
    args.inputPath = in.string();
    args.outputPath = out.string();
    args.quality = "Performance";   // 2.0x -> 160x120

    std::ostringstream o, e;
    ASSERT_EQ(RunUpscale(args, o, e), 0) << e.str();
    ASSERT_TRUE(fs::exists(out)) << "no output file was written";

    // Read the actual written file back — this exercises encode + decode too.
    csr::LoadResult loaded = csr::LoadImage(out.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.image.width, 160);
    EXPECT_EQ(loaded.image.height, 120);

    const csr::Image bilinear = csr::Bilinear(low, 160, 120);
    const double csrPsnr = Psnr(truth, loaded.image);
    const double bilPsnr = Psnr(truth, bilinear);

    o << "CSR " << csrPsnr << " dB vs bilinear " << bilPsnr << " dB\n";
    // The whole point: CSR reconstructs the edge better than a plain resize.
    // (Compared after an 8-bit round trip, so the margin is real, not numeric.)
    EXPECT_GT(csrPsnr, bilPsnr)
        << "CSR (" << csrPsnr << " dB) did not beat bilinear (" << bilPsnr << " dB)";

    std::error_code ec; fs::remove_all(dir, ec);
}

TEST(UpscaleCli, ExplicitDimensionsOverrideQuality) {
    const fs::path dir = TempDir();
    const fs::path in = dir / "in.png", out = dir / "out.png";
    std::string werr;
    ASSERT_TRUE(csr::SavePng(Scene(50, 40), in.string(), werr)) << werr;

    UpscaleArgs args;
    args.inputPath = in.string();
    args.outputPath = out.string();
    args.outWidth = 137;
    args.outHeight = 111;

    std::ostringstream o, e;
    ASSERT_EQ(RunUpscale(args, o, e), 0) << e.str();
    csr::LoadResult loaded = csr::LoadImage(out.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.image.width, 137);
    EXPECT_EQ(loaded.image.height, 111);
    fs::remove_all(dir);
}

TEST(UpscaleCli, MissingInputFailsCleanlyWithoutWriting) {
    const fs::path dir = TempDir();
    UpscaleArgs args;
    args.inputPath = (dir / "does-not-exist.png").string();
    args.outputPath = (dir / "out.png").string();
    std::ostringstream o, e;
    EXPECT_NE(RunUpscale(args, o, e), 0);
    EXPECT_FALSE(fs::exists(dir / "out.png"));
    EXPECT_FALSE(e.str().empty());
    fs::remove_all(dir);
}

TEST(UpscaleCli, ArgParsingAcceptsRealInvocationsAndRejectsGarbage) {
    UpscaleArgs a; std::string err; bool help = false;

    ASSERT_TRUE(ParseUpscaleArgs({"in.png", "out.png", "--quality", "Balanced"}, a, err, help)) << err;
    EXPECT_EQ(a.inputPath, "in.png");
    EXPECT_EQ(a.outputPath, "out.png");
    EXPECT_EQ(a.quality, "Balanced");
    EXPECT_FALSE(help);

    ASSERT_TRUE(ParseUpscaleArgs({"--help"}, a, err, help));
    EXPECT_TRUE(help);

    EXPECT_FALSE(ParseUpscaleArgs({"only-one-path.png"}, a, err, help));
    EXPECT_FALSE(ParseUpscaleArgs({"in.png", "out.png", "--quality", "Nonsense"}, a, err, help));
    EXPECT_FALSE(ParseUpscaleArgs({"in.png", "out.png", "--sharpness", "abc"}, a, err, help));
    EXPECT_FALSE(ParseUpscaleArgs({"in.png", "out.png", "--frobnicate"}, a, err, help));
}
