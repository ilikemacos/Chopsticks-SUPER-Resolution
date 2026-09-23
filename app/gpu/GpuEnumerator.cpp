#include "GpuEnumerator.h"
#include "MockGpuEnumerator.h"
#ifdef _WIN32
#include "DxgiGpuEnumerator.h"
#endif

namespace ufx {

const char* VendorName(GpuVendor v) {
    switch (v) {
        case GpuVendor::Nvidia:    return "NVIDIA";
        case GpuVendor::Amd:       return "AMD";
        case GpuVendor::Intel:     return "Intel";
        case GpuVendor::Microsoft: return "Microsoft Basic Render";
        default:                   return "Unknown";
    }
}

const char* ArchName(GpuArch a) {
    switch (a) {
        case GpuArch::Gcn:      return "GCN";
        case GpuArch::Rdna1:    return "RDNA 1";
        case GpuArch::Rdna2:    return "RDNA 2";
        case GpuArch::Rdna3:    return "RDNA 3";
        case GpuArch::Rdna4:    return "RDNA 4";
        case GpuArch::Pascal:   return "Pascal";
        case GpuArch::Turing:   return "Turing";
        case GpuArch::Ampere:   return "Ampere";
        case GpuArch::Ada:      return "Ada Lovelace";
        case GpuArch::Blackwell:return "Blackwell";
        case GpuArch::XeLp:     return "Xe-LP";
        case GpuArch::XeHpg:    return "Xe-HPG (Arc Alchemist)";
        case GpuArch::XeHpc:    return "Xe-HPC";
        case GpuArch::Xe2:      return "Xe2 (Battlemage)";
        default:                return "Unknown";
    }
}

std::unique_ptr<IGpuEnumerator> CreateDefaultGpuEnumerator() {
#ifdef _WIN32
    return std::make_unique<DxgiGpuEnumerator>();
#else
    return std::make_unique<MockGpuEnumerator>(MockGpuEnumerator::Default());
#endif
}

}
