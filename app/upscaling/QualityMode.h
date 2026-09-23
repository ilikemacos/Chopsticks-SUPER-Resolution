#pragma once

#include <string>

namespace ufx {

enum class QualityMode {
    Native,
    UltraQuality,
    Quality,
    Balanced,
    Performance,
    UltraPerformance,
};

struct ScaleRatio { float x; float y; };

// The ratios below are the ones each vendor officially publishes.
// FSR 3 quality-mode ratios (source: AMD FidelityFX SDK docs)
constexpr ScaleRatio FsrRatio(QualityMode m) {
    switch (m) {
        case QualityMode::Native:           return {1.00f, 1.00f};
        case QualityMode::UltraQuality:     return {1.30f, 1.30f}; // FSR "Native AA" ≈ 1.0, Ultra Quality historical ≈ 1.3
        case QualityMode::Quality:          return {1.50f, 1.50f};
        case QualityMode::Balanced:         return {1.70f, 1.70f};
        case QualityMode::Performance:      return {2.00f, 2.00f};
        case QualityMode::UltraPerformance: return {3.00f, 3.00f};
    }
    return {1.f, 1.f};
}

// XeSS 1.x quality-mode ratios (source: Intel XeSS SDK docs)
constexpr ScaleRatio XessRatio(QualityMode m) {
    switch (m) {
        case QualityMode::Native:           return {1.00f, 1.00f};
        case QualityMode::UltraQuality:     return {1.30f, 1.30f};
        case QualityMode::Quality:          return {1.50f, 1.50f};
        case QualityMode::Balanced:         return {1.70f, 1.70f};
        case QualityMode::Performance:      return {2.00f, 2.00f};
        case QualityMode::UltraPerformance: return {3.00f, 3.00f};
    }
    return {1.f, 1.f};
}

const char* QualityModeName(QualityMode m);
bool ParseQualityMode(std::string_view s, QualityMode& out);

}
