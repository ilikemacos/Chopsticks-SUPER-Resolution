#include "MockGpuEnumerator.h"

namespace ufx {

MockGpuEnumerator MockGpuEnumerator::Default() {
    GpuInfo a{};
    a.name = "Mock NVIDIA RTX 4070";
    a.vendor = GpuVendor::Nvidia;
    a.arch = GpuArch::Ada;
    a.vendorId = 0x10DE;
    a.deviceId = 0x2786;
    a.dedicatedVramBytes = 12ull * 1024 * 1024 * 1024;
    a.driverVersion = "555.99.0.0";
    a.driverVendor = "NVIDIA";
    a.supportsD3D11 = true;
    a.supportsD3D12 = true;
    a.d3d11FeatureLevel = 0xC000; // 12_0
    a.d3d12FeatureLevel = 0xC200; // 12_2
    a.supportsVulkan = true;
    a.vulkanApiVersion = "1.3.275";
    return MockGpuEnumerator({a});
}

}
