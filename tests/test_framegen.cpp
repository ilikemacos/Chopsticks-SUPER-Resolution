#include <gtest/gtest.h>

#include "framegen/FrameGenRegistry.h"

using namespace ufx;

namespace {
GpuInfo Dx12Gpu() {
    GpuInfo g{};
    g.vendor = GpuVendor::Amd; g.arch = GpuArch::Rdna3;
    g.supportsD3D12 = true;
    return g;
}
}

TEST(FrameGen, DefaultStateRequiresGameIntegration) {
    FrameGenRegistry reg;
    auto* fsrfg = reg.Find(FrameGenId::Fsr3Fg);
    ASSERT_NE(fsrfg, nullptr);
    // Without the game declaring support, we must NOT claim it works.
    auto state = fsrfg->Evaluate(Dx12Gpu(), GraphicsApi::D3D12, /*gameDeclaresSupport=*/false);
    EXPECT_EQ(state, FrameGenState::RequiresGameIntegration);
}

TEST(FrameGen, SupportedWhenGameDeclares) {
    FrameGenRegistry reg;
    auto* fsrfg = reg.Find(FrameGenId::Fsr3Fg);
    auto state = fsrfg->Evaluate(Dx12Gpu(), GraphicsApi::D3D12, /*gameDeclaresSupport=*/true);
    EXPECT_EQ(state, FrameGenState::Supported);
}

TEST(FrameGen, Dx11IsIncompatible) {
    FrameGenRegistry reg;
    auto* fsrfg = reg.Find(FrameGenId::Fsr3Fg);
    auto state = fsrfg->Evaluate(Dx12Gpu(), GraphicsApi::D3D11, true);
    EXPECT_EQ(state, FrameGenState::RequiresCompatibleImpl);
}

TEST(FrameGen, ExternalLimitationIsDocumented) {
    FrameGenRegistry reg;
    for (const auto& gen : reg.All()) {
        EXPECT_FALSE(gen->Caps().externalLimitation.empty())
            << "every frame generator must document its external limitation";
    }
}

TEST(FrameGen, StateLabels) {
    EXPECT_STREQ(FrameGenStateLabel(FrameGenState::Supported), "Supported");
    EXPECT_STREQ(FrameGenStateLabel(FrameGenState::RequiresGameIntegration), "Requires game integration");
}

// CSR frame generation is external: unlike FSR/XeSS FG it does NOT require the
// game to integrate anything, so it is Supported without gameDeclaresSupport.
TEST(FrameGen, CsrExternalSupportedWithoutGameIntegration) {
    FrameGenRegistry reg;
    auto* csr = reg.Find(FrameGenId::CsrInterpolated);
    ASSERT_NE(csr, nullptr);
    auto state = csr->Evaluate(Dx12Gpu(), GraphicsApi::D3D12, /*gameDeclaresSupport=*/false);
    EXPECT_EQ(state, FrameGenState::Supported);
    // ...and it works on a plain D3D11 app too, since it captures finished frames.
    GpuInfo dx11{}; dx11.supportsD3D11 = true;
    EXPECT_EQ(csr->Evaluate(dx11, GraphicsApi::D3D11, false), FrameGenState::Supported);
}

// Its honesty note must name the latency cost so the UI never sells it as free FPS.
TEST(FrameGen, CsrLimitationNamesLatencyCost) {
    FrameGenRegistry reg;
    auto* csr = reg.Find(FrameGenId::CsrInterpolated);
    ASSERT_NE(csr, nullptr);
    EXPECT_NE(csr->Caps().externalLimitation.find("latency"), std::string::npos);
}

TEST(FrameGen, ParsesCsrFgId) {
    FrameGenId id{};
    ASSERT_TRUE(ParseFrameGenId("CSR-FG", id));
    EXPECT_EQ(id, FrameGenId::CsrInterpolated);
    EXPECT_STREQ(FrameGenIdName(FrameGenId::CsrInterpolated), "CSR-FG");
}
