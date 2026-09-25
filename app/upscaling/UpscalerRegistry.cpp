#include "UpscalerRegistry.h"

#include "CsrUpscaler.h"
#include "FsrUpscaler.h"
#include "XessUpscaler.h"
#include "NullUpscaler.h"

#include <array>
#include <cstring>

namespace ufx {

UpscalerRegistry::UpscalerRegistry() {
    upscalers_.push_back(std::make_unique<NullUpscaler>());
    // CSR first among the real entries: it is the one that needs no game support.
    upscalers_.push_back(std::make_unique<CsrUpscaler>());
    upscalers_.push_back(std::make_unique<FsrUpscaler>(1));
    upscalers_.push_back(std::make_unique<FsrUpscaler>(2));
    upscalers_.push_back(std::make_unique<FsrUpscaler>(3));
    upscalers_.push_back(std::make_unique<FsrUpscaler>(4));
    upscalers_.push_back(std::make_unique<XessUpscaler>());
}

IUpscaler* UpscalerRegistry::Find(UpscalerId id) const {
    for (const auto& u : upscalers_)
        if (u->Caps().id == id) return u.get();
    return nullptr;
}

std::vector<UpscalerRegistry::Entry>
UpscalerRegistry::Evaluate(const GpuInfo& gpu, GraphicsApi api) const {
    std::vector<Entry> out;
    out.reserve(upscalers_.size());
    for (const auto& u : upscalers_)
        out.push_back({u.get(), u->CheckAvailability(gpu, api)});
    return out;
}

const char* QualityModeName(QualityMode m) {
    switch (m) {
        case QualityMode::Native:           return "Native";
        case QualityMode::UltraQuality:     return "Ultra Quality";
        case QualityMode::Quality:          return "Quality";
        case QualityMode::Balanced:         return "Balanced";
        case QualityMode::Performance:      return "Performance";
        case QualityMode::UltraPerformance: return "Ultra Performance";
    }
    return "Unknown";
}

bool ParseQualityMode(std::string_view s, QualityMode& out) {
    struct M { const char* n; QualityMode v; };
    static const M table[] = {
        {"Native", QualityMode::Native},
        {"Ultra Quality", QualityMode::UltraQuality},
        {"Quality", QualityMode::Quality},
        {"Balanced", QualityMode::Balanced},
        {"Performance", QualityMode::Performance},
        {"Ultra Performance", QualityMode::UltraPerformance},
    };
    for (const auto& m : table) if (s == m.n) { out = m.v; return true; }
    return false;
}

const char* UpscalerIdName(UpscalerId id) {
    switch (id) {
        case UpscalerId::None: return "None";
        case UpscalerId::Csr:  return "CSR";
        case UpscalerId::Fsr1: return "FSR1";
        case UpscalerId::Fsr2: return "FSR2";
        case UpscalerId::Fsr3: return "FSR3";
        case UpscalerId::Fsr4: return "FSR4";
        case UpscalerId::XeSS: return "XeSS";
    }
    return "None";
}

bool ParseUpscalerId(std::string_view s, UpscalerId& out) {
    struct M { const char* n; UpscalerId v; };
    static const M table[] = {
        {"None", UpscalerId::None}, {"CSR", UpscalerId::Csr},
        {"FSR1", UpscalerId::Fsr1},
        {"FSR2", UpscalerId::Fsr2}, {"FSR3", UpscalerId::Fsr3},
        {"FSR4", UpscalerId::Fsr4}, {"XeSS", UpscalerId::XeSS},
    };
    for (const auto& m : table) if (s == m.n) { out = m.v; return true; }
    return false;
}

const char* GraphicsApiName(GraphicsApi api) {
    switch (api) {
        case GraphicsApi::D3D11:  return "DirectX 11";
        case GraphicsApi::D3D12:  return "DirectX 12";
        case GraphicsApi::Vulkan: return "Vulkan";
    }
    return "Unknown";
}

bool ParseGraphicsApi(std::string_view s, GraphicsApi& out) {
    if (s == "DirectX 11" || s == "DX11" || s == "D3D11") { out = GraphicsApi::D3D11; return true; }
    if (s == "DirectX 12" || s == "DX12" || s == "D3D12") { out = GraphicsApi::D3D12; return true; }
    if (s == "Vulkan" || s == "VK")                       { out = GraphicsApi::Vulkan; return true; }
    return false;
}

}
