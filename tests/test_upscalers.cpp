#include <gtest/gtest.h>

#include "upscaling/UpscalerRegistry.h"

using namespace ufx;

namespace {
GpuInfo Amd(GpuArch arch) {
    GpuInfo g{};
    g.vendor = GpuVendor::Amd; g.arch = arch;
    g.supportsD3D11 = g.supportsD3D12 = g.supportsVulkan = true;
    return g;
}
GpuInfo Nvidia() {
    GpuInfo g{};
    g.vendor = GpuVendor::Nvidia; g.arch = GpuArch::Ada;
    g.supportsD3D11 = g.supportsD3D12 = g.supportsVulkan = true;
    return g;
}
}

TEST(Upscalers, RegistryHasAllAdapters) {
    UpscalerRegistry reg;
    EXPECT_NE(reg.Find(UpscalerId::None), nullptr);
    EXPECT_NE(reg.Find(UpscalerId::Fsr1), nullptr);
    EXPECT_NE(reg.Find(UpscalerId::Fsr3), nullptr);
    EXPECT_NE(reg.Find(UpscalerId::Fsr4), nullptr);
    EXPECT_NE(reg.Find(UpscalerId::XeSS), nullptr);
}

TEST(Upscalers, Fsr4RequiresRdna4) {
    UpscalerRegistry reg;
    auto* fsr4 = reg.Find(UpscalerId::Fsr4);
    ASSERT_NE(fsr4, nullptr);

    // Non-AMD GPU: unavailable with a clear reason.
    auto onNvidia = fsr4->CheckAvailability(Nvidia(), GraphicsApi::D3D12);
    EXPECT_FALSE(onNvidia.available);
    EXPECT_NE(onNvidia.reason.find("RDNA 4"), std::string::npos);

    // AMD RDNA 3: still unavailable.
    EXPECT_FALSE(fsr4->CheckAvailability(Amd(GpuArch::Rdna3), GraphicsApi::D3D12).available);

    // AMD RDNA 4: available.
    EXPECT_TRUE(fsr4->CheckAvailability(Amd(GpuArch::Rdna4), GraphicsApi::D3D12).available);
}

TEST(Upscalers, Fsr3CrossVendor) {
    UpscalerRegistry reg;
    auto* fsr3 = reg.Find(UpscalerId::Fsr3);
    EXPECT_TRUE(fsr3->CheckAvailability(Nvidia(), GraphicsApi::D3D12).available);
    EXPECT_TRUE(fsr3->CheckAvailability(Amd(GpuArch::Rdna2), GraphicsApi::D3D12).available);
}

TEST(Upscalers, RenderResolutionMath) {
    UpscalerRegistry reg;
    auto* fsr3 = reg.Find(UpscalerId::Fsr3);
    auto r = fsr3->RenderResolutionFor(2560, 1440, QualityMode::Quality);
    ASSERT_TRUE(r.has_value());
    // Quality == 1.5x scale → 1707 x 960.
    EXPECT_EQ(r->first, 1707u);
    EXPECT_EQ(r->second, 960u);

    auto perf = fsr3->RenderResolutionFor(2560, 1440, QualityMode::Performance);
    ASSERT_TRUE(perf.has_value());
    EXPECT_EQ(perf->first, 1280u);
    EXPECT_EQ(perf->second, 720u);
}

TEST(Upscalers, UnsupportedModeReturnsNullopt) {
    UpscalerRegistry reg;
    auto* fsr4 = reg.Find(UpscalerId::Fsr4);
    // FSR4 does not expose UltraPerformance in our caps.
    EXPECT_FALSE(fsr4->RenderResolutionFor(2560, 1440, QualityMode::UltraPerformance).has_value());
}

TEST(Upscalers, NullUpscalerIsNativeOnly) {
    UpscalerRegistry reg;
    auto* none = reg.Find(UpscalerId::None);
    auto r = none->RenderResolutionFor(1920, 1080, QualityMode::Native);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->first, 1920u);
    EXPECT_EQ(r->second, 1080u);
    EXPECT_FALSE(none->RenderResolutionFor(1920, 1080, QualityMode::Quality).has_value());
}

TEST(Upscalers, ParseAndNameRoundTrip) {
    for (auto id : {UpscalerId::None, UpscalerId::Fsr1, UpscalerId::Fsr2,
                    UpscalerId::Fsr3, UpscalerId::Fsr4, UpscalerId::XeSS}) {
        UpscalerId parsed;
        ASSERT_TRUE(ParseUpscalerId(UpscalerIdName(id), parsed));
        EXPECT_EQ(parsed, id);
    }
}
