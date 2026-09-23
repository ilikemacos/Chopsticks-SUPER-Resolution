#pragma once

#include "upscaling/UpscalerRegistry.h"
#include "framegen/IFrameGenerator.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

namespace ufx {

struct GameProfile {
    std::string name;                 // "Cyber Example"
    std::string executablePath;       // full path to the game .exe
    GraphicsApi api = GraphicsApi::D3D12;
    UpscalerId upscaler = UpscalerId::None;
    QualityMode quality = QualityMode::Quality;
    FrameGenId frameGen = FrameGenId::None;
    bool frameGenEnabled = false;
    float sharpness = 0.5f;           // 0..1, only if the upscaler supports it
    uint32_t outputWidth = 2560;
    uint32_t outputHeight = 1440;
    std::optional<int> fpsLimit;      // nullopt == unlimited
    // Whether the game itself declares native support for the chosen tech.
    // Set by the user or by a known-games database; never assumed true.
    bool gameDeclaresUpscalerSupport = false;
    bool gameDeclaresFrameGenSupport = false;
    // Free-form additional options stored verbatim.
    nlohmann::json extra = nlohmann::json::object();
};

nlohmann::json ToJson(const GameProfile& p);
std::optional<GameProfile> FromJson(const nlohmann::json& j, std::string* error);

}
