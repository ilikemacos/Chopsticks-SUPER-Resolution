#include <gtest/gtest.h>

#include "gpu/MockGpuEnumerator.h"
#include "gpu/GpuEnumerator.h"

using namespace ufx;

TEST(Gpu, VendorFromIdKnownVendors) {
    EXPECT_EQ(VendorFromId(0x10DE), GpuVendor::Nvidia);
    EXPECT_EQ(VendorFromId(0x1002), GpuVendor::Amd);
    EXPECT_EQ(VendorFromId(0x8086), GpuVendor::Intel);
    EXPECT_EQ(VendorFromId(0x1414), GpuVendor::Microsoft);
    EXPECT_EQ(VendorFromId(0xDEAD), GpuVendor::Unknown);
}

TEST(Gpu, MockEnumeratorReturnsConfiguredGpu) {
    MockGpuEnumerator mock = MockGpuEnumerator::Default();
    auto gpus = mock.Enumerate();
    ASSERT_EQ(gpus.size(), 1u);
    EXPECT_EQ(gpus[0].vendor, GpuVendor::Nvidia);
    EXPECT_TRUE(gpus[0].supportsD3D12);
    EXPECT_GT(gpus[0].dedicatedVramBytes, 0u);
}

TEST(Gpu, VendorAndArchNamesAreStable) {
    EXPECT_STREQ(VendorName(GpuVendor::Amd), "AMD");
    EXPECT_STREQ(ArchName(GpuArch::Rdna4), "RDNA 4");
    EXPECT_STREQ(ArchName(GpuArch::Unknown), "Unknown");
}

TEST(Gpu, CustomMockGpuList) {
    GpuInfo intel{};
    intel.name = "Arc B580";
    intel.vendor = GpuVendor::Intel;
    intel.arch = GpuArch::Xe2;
    intel.supportsD3D12 = true;
    MockGpuEnumerator mock({intel});
    auto gpus = mock.Enumerate();
    ASSERT_EQ(gpus.size(), 1u);
    EXPECT_EQ(gpus[0].arch, GpuArch::Xe2);
}
