// Verifies the C++ CSR port against csr/ref/golden/*.json — the same vectors the
// Python reference emits and the C# port is held to.
//
// A failure here means this port diverged from the specification. Historically
// the likely culprits are the luma weights, the deringing clamp, or edge
// handling at frame borders.

#include "upscaling/CsrUpscaler.h"
#include "upscaling/UpscalerRegistry.h"
#include "upscaling/csr/CsrCore.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace {

using nlohmann::json;

json LoadJson(const std::filesystem::path& p) {
    std::ifstream f(p);
    EXPECT_TRUE(f.good()) << "cannot open " << p.string();
    json j;
    f >> j;
    return j;
}

std::filesystem::path GoldenDir() { return std::filesystem::path(UFX_CSR_GOLDEN_DIR); }

ufx::csr::Image ImageFrom(const json& plane) {
    ufx::csr::Image img(plane.at("width").get<int>(), plane.at("height").get<int>());
    const auto rgb = plane.at("rgb").get<std::vector<float>>();
    EXPECT_EQ(rgb.size(), img.data.size());
    img.data = rgb;
    return img;
}

// Worst absolute per-channel difference, so a failure reports how far off it is
// rather than merely that it is off.
float WorstDiff(const json& expected, const ufx::csr::Image& actual) {
    const auto rgb = expected.at("rgb").get<std::vector<float>>();
    EXPECT_EQ(expected.at("width").get<int>(), actual.width);
    EXPECT_EQ(expected.at("height").get<int>(), actual.height);
    EXPECT_EQ(rgb.size(), actual.data.size());
    float worst = 0.0f;
    for (size_t i = 0; i < rgb.size() && i < actual.data.size(); ++i)
        worst = std::max(worst, std::fabs(rgb[i] - actual.data[i]));
    return worst;
}

}  // namespace

// The case list comes from index.json rather than a directory glob: globbing
// would let a missing case read as a pass.
TEST(CsrGolden, EveryExportedCaseMatchesTheReference) {
    const json index = LoadJson(GoldenDir() / "index.json");
    const float tolerance = index.at("tolerance").get<float>();
    const auto& cases = index.at("cases");
    ASSERT_FALSE(cases.empty()) << "golden/index.json lists no cases";

    for (const auto& entry : cases) {
        const std::string name = entry.at("name").get<std::string>();
        const std::filesystem::path file = GoldenDir() / (name + ".json");
        ASSERT_TRUE(std::filesystem::exists(file))
            << "index.json lists '" << name << "' but " << name
            << ".json is missing. Run 'python3 export_golden.py' in csr/ref.";

        const json c = LoadJson(file);
        const ufx::csr::Image src = ImageFrom(c.at("input"));
        ufx::csr::Options o;
        o.sharpnessStops = c.at("config").at("sharpnessStops").get<float>();
        o.adaptiveSharpen = c.at("config").at("adaptiveSharpen").get<bool>();
        o.dering = c.at("config").at("dering").get<bool>();

        const int ow = c.at("resolve").at("width").get<int>();
        const int oh = c.at("resolve").at("height").get<int>();

        const ufx::csr::Image resolved = ufx::csr::Resolve(src, ow, oh, o);
        EXPECT_LE(WorstDiff(c.at("resolve"), resolved), tolerance)
            << name << ": resolve pass diverged";

        const ufx::csr::Image full = ufx::csr::Sharpen(resolved, o);
        EXPECT_LE(WorstDiff(c.at("full"), full), tolerance)
            << name << ": full pipeline diverged";
    }
}

TEST(CsrGolden, BilinearBaselineMatchesTheReference) {
    const json b = LoadJson(GoldenDir() / "bilinear_baseline.json");
    const float tolerance = b.at("tolerance").get<float>();
    ASSERT_FALSE(b.at("cases").empty());
    for (const auto& c : b.at("cases")) {
        const ufx::csr::Image src = ImageFrom(c.at("input"));
        const auto& want = c.at("bilinear");
        const ufx::csr::Image got = ufx::csr::Bilinear(
            src, want.at("width").get<int>(), want.at("height").get<int>());
        EXPECT_LE(WorstDiff(want, got), tolerance)
            << c.at("name").get<std::string>() << ": bilinear baseline diverged";
    }
}

TEST(CsrCore, SolidColourSurvivesUpscalingExactly) {
    ufx::csr::Image src(5, 4);
    for (size_t i = 0; i < src.data.size(); i += 3) {
        src.data[i] = 0.25f; src.data[i + 1] = 0.5f; src.data[i + 2] = 0.75f;
    }
    const ufx::csr::Image out = ufx::csr::Upscale(src, 11, 9);
    for (size_t i = 0; i < out.data.size(); i += 3) {
        EXPECT_NEAR(out.data[i], 0.25f, 1e-5f);
        EXPECT_NEAR(out.data[i + 1], 0.5f, 1e-5f);
        EXPECT_NEAR(out.data[i + 2], 0.75f, 1e-5f);
    }
}

TEST(CsrCore, OneToOneIsBypassedRatherThanFiltered) {
    // The resolve has a negative lobe, so running it at 1:1 would alter a frame
    // the user asked not to be upscaled. Upscale must hand back the input.
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    ufx::csr::Image src(9, 6);
    for (auto& v : src.data) v = d(rng);

    const ufx::csr::Image out = ufx::csr::Upscale(src, 9, 6);
    for (size_t i = 0; i < src.data.size(); ++i) EXPECT_FLOAT_EQ(out.data[i], src.data[i]);
}

TEST(CsrCore, DeringClampKeepsOutputWithinTheLocalTapRange) {
    // Worst case for overshoot: an isolated midtone feature, where the final
    // [0,1] clip cannot mask a ringing artefact.
    ufx::csr::Image src(10, 10);
    for (size_t i = 0; i < src.data.size(); ++i) src.data[i] = 0.35f;
    for (int y = 4; y < 6; ++y)
        for (int x = 4; x < 6; ++x)
            for (int c = 0; c < 3; ++c) src.data[src.ClampedIndex(x, y) + c] = 0.75f;

    const ufx::csr::Image out = ufx::csr::Resolve(src, 15, 15);
    for (const float v : out.data) {
        EXPECT_GE(v, 0.35f - 1e-4f);
        EXPECT_LE(v, 0.75f + 1e-4f);
    }
}

TEST(CsrCore, IsDeterministic) {
    std::mt19937 rng(11);
    std::uniform_real_distribution<float> d(0.0f, 1.0f);
    ufx::csr::Image src(12, 12);
    for (auto& v : src.data) v = d(rng);
    const ufx::csr::Image a = ufx::csr::Upscale(src, 16, 16);
    const ufx::csr::Image b = ufx::csr::Upscale(src, 16, 16);
    ASSERT_EQ(a.data.size(), b.data.size());
    for (size_t i = 0; i < a.data.size(); ++i) EXPECT_FLOAT_EQ(a.data[i], b.data[i]);
}

// --- the registry side: CSR must be reachable as an upscaler, not just as maths ---

TEST(CsrRegistry, IsRegisteredAndNeedsNoGameIntegration) {
    const ufx::UpscalerRegistry reg;
    ufx::IUpscaler* csr = reg.Find(ufx::UpscalerId::Csr);
    ASSERT_NE(csr, nullptr) << "CSR is not in the registry";

    const ufx::UpscalerCaps caps = csr->Caps();
    EXPECT_EQ(caps.id, ufx::UpscalerId::Csr);
    // This is the property that makes CSR different from every other entry.
    EXPECT_FALSE(caps.gameIntegrationRequired);
    EXPECT_TRUE(caps.sharpnessSupported);
    EXPECT_FALSE(caps.displayName.empty());
}

TEST(CsrRegistry, IdRoundTripsThroughNameAndParse) {
    EXPECT_STREQ(ufx::UpscalerIdName(ufx::UpscalerId::Csr), "CSR");
    ufx::UpscalerId id{};
    ASSERT_TRUE(ufx::ParseUpscalerId("CSR", id));
    EXPECT_EQ(id, ufx::UpscalerId::Csr);
}

TEST(CsrRegistry, IsAvailableWithoutAnyVendorHardware) {
    const ufx::UpscalerRegistry reg;
    ufx::IUpscaler* csr = reg.Find(ufx::UpscalerId::Csr);
    ASSERT_NE(csr, nullptr);
    ufx::GpuInfo gpu{};              // deliberately blank: no vendor, no features
    const ufx::Availability a = csr->CheckAvailability(gpu, ufx::GraphicsApi::D3D11);
    EXPECT_TRUE(a.available) << a.reason;
}

TEST(CsrRegistry, RenderResolutionMatchesThePresetLadderAndSkipsNative) {
    const ufx::UpscalerRegistry reg;
    ufx::IUpscaler* csr = reg.Find(ufx::UpscalerId::Csr);
    ASSERT_NE(csr, nullptr);

    const auto quality = csr->RenderResolutionFor(2560, 1440, ufx::QualityMode::Quality);
    ASSERT_TRUE(quality.has_value());
    EXPECT_EQ(quality->first, 1707u);   // 2560 / 1.5
    EXPECT_EQ(quality->second, 960u);   // 1440 / 1.5

    const auto perf = csr->RenderResolutionFor(1920, 1080, ufx::QualityMode::Performance);
    ASSERT_TRUE(perf.has_value());
    EXPECT_EQ(perf->first, 960u);
    EXPECT_EQ(perf->second, 540u);

    // Native is not offered, because at 1:1 CSR does nothing.
    EXPECT_FALSE(csr->RenderResolutionFor(1920, 1080, ufx::QualityMode::Native).has_value());
}

TEST(CsrRegistry, RunActuallyUpscalesThroughTheAdapter) {
    ufx::csr::Image src(8, 8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            for (int c = 0; c < 3; ++c)
                src.data[src.ClampedIndex(x, y) + c] = x < 4 ? 0.2f : 0.8f;

    const ufx::csr::Image out = ufx::CsrUpscaler::Run(src, 16, 16);
    EXPECT_EQ(out.width, 16);
    EXPECT_EQ(out.height, 16);
    // The edge must survive: left side dark, right side light.
    EXPECT_LT(out.data[out.ClampedIndex(1, 8)], 0.35f);
    EXPECT_GT(out.data[out.ClampedIndex(14, 8)], 0.65f);
}


// The row-parallel Resolve/Sharpen must produce output that does not depend on
// the thread count, or the golden vectors would be a lie on multi-core machines.
TEST(CsrCore, OutputIsIdenticalRegardlessOfThreadCount) {
    std::mt19937 rng(20);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    ufx::csr::Image src(60, 48);   // big enough to cross the parallel threshold
    for (auto& v : src.data) v = dist(rng);

#if defined(_WIN32)
    _putenv_s("CSR_THREADS", "1");
#else
    setenv("CSR_THREADS", "1", 1);
#endif
    const ufx::csr::Image serial = ufx::csr::Upscale(src, 120, 96);

#if defined(_WIN32)
    _putenv_s("CSR_THREADS", "4");
#else
    setenv("CSR_THREADS", "4", 1);
#endif
    const ufx::csr::Image parallel = ufx::csr::Upscale(src, 120, 96);

#if defined(_WIN32)
    _putenv_s("CSR_THREADS", "");
#else
    unsetenv("CSR_THREADS");
#endif

    ASSERT_EQ(serial.data.size(), parallel.data.size());
    for (size_t i = 0; i < serial.data.size(); ++i)
        EXPECT_EQ(serial.data[i], parallel.data[i]) << "thread count changed pixel " << i;
}
