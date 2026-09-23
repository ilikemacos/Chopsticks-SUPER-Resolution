#include "ProfileService.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace ufx {

namespace fs = std::filesystem;

namespace {

std::string ToStr(GraphicsApi a) { return GraphicsApiName(a); }

const char* QualityKey(QualityMode m) { return QualityModeName(m); }

}

nlohmann::json ToJson(const GameProfile& p) {
    nlohmann::json j;
    j["schema"] = 1;
    j["name"] = p.name;
    j["executablePath"] = p.executablePath;
    j["api"] = GraphicsApiName(p.api);
    j["upscaler"] = UpscalerIdName(p.upscaler);
    j["quality"] = QualityModeName(p.quality);
    j["frameGen"] = FrameGenIdName(p.frameGen);
    j["frameGenEnabled"] = p.frameGenEnabled;
    j["sharpness"] = p.sharpness;
    j["outputWidth"] = p.outputWidth;
    j["outputHeight"] = p.outputHeight;
    if (p.fpsLimit) j["fpsLimit"] = *p.fpsLimit;
    else            j["fpsLimit"] = nullptr;
    j["gameDeclaresUpscalerSupport"] = p.gameDeclaresUpscalerSupport;
    j["gameDeclaresFrameGenSupport"] = p.gameDeclaresFrameGenSupport;
    j["extra"] = p.extra;
    return j;
}

std::optional<GameProfile> FromJson(const nlohmann::json& j, std::string* error) {
    auto fail = [&](const std::string& m) -> std::optional<GameProfile> {
        if (error) *error = m;
        return std::nullopt;
    };
    if (!j.is_object()) return fail("profile root is not a JSON object");
    if (!j.contains("name") || !j["name"].is_string()) return fail("missing 'name'");

    GameProfile p;
    p.name = j.value("name", "");
    p.executablePath = j.value("executablePath", "");

    if (auto s = j.value("api", std::string("DirectX 12")); !ParseGraphicsApi(s, p.api))
        return fail("invalid 'api': " + s);
    if (auto s = j.value("upscaler", std::string("None")); !ParseUpscalerId(s, p.upscaler))
        return fail("invalid 'upscaler': " + s);
    if (auto s = j.value("quality", std::string("Quality")); !ParseQualityMode(s, p.quality))
        return fail("invalid 'quality': " + s);
    if (auto s = j.value("frameGen", std::string("None")); !ParseFrameGenId(s, p.frameGen))
        return fail("invalid 'frameGen': " + s);

    p.frameGenEnabled = j.value("frameGenEnabled", false);
    p.sharpness = j.value("sharpness", 0.5f);
    p.outputWidth = j.value("outputWidth", 2560u);
    p.outputHeight = j.value("outputHeight", 1440u);
    if (j.contains("fpsLimit") && j["fpsLimit"].is_number_integer())
        p.fpsLimit = j["fpsLimit"].get<int>();
    p.gameDeclaresUpscalerSupport = j.value("gameDeclaresUpscalerSupport", false);
    p.gameDeclaresFrameGenSupport = j.value("gameDeclaresFrameGenSupport", false);
    if (j.contains("extra") && j["extra"].is_object()) p.extra = j["extra"];

    if (p.outputWidth == 0 || p.outputHeight == 0)
        return fail("output resolution must be non-zero");
    if (p.sharpness < 0.f || p.sharpness > 1.f)
        return fail("sharpness must be within 0..1");

    return p;
}

ProfileService::ProfileService(AppPaths paths) : paths_(std::move(paths)) {
    paths_.ensure();
}

std::string ProfileService::Slug(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) out.push_back(static_cast<char>(std::tolower(uc)));
        else if (c == ' ' || c == '-' || c == '_') out.push_back('-');
    }
    if (out.empty()) out = "profile";
    return out;
}

fs::path ProfileService::FileFor(const std::string& name) const {
    return paths_.profiles / (Slug(name) + ".json");
}

Result<std::vector<GameProfile>> ProfileService::LoadAll() const {
    std::vector<GameProfile> profiles;
    std::error_code ec;
    if (!fs::exists(paths_.profiles, ec)) return profiles;
    for (const auto& entry : fs::directory_iterator(paths_.profiles, ec)) {
        if (ec) break;
        if (entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path());
        if (!in) continue;
        nlohmann::json j;
        try { in >> j; }
        catch (const std::exception& e) {
            return Error{"parse_error", std::string(entry.path().filename().string()) + ": " + e.what()};
        }
        std::string err;
        auto p = FromJson(j, &err);
        if (!p) return Error{"invalid_profile", entry.path().filename().string() + ": " + err};
        profiles.push_back(std::move(*p));
    }
    return profiles;
}

Result<void> ProfileService::Save(const GameProfile& profile) const {
    if (profile.name.empty()) return Error{"invalid_argument", "profile name is empty"};
    std::ofstream out(FileFor(profile.name), std::ios::trunc);
    if (!out) return Error{"io_error", "cannot open profile file for writing"};
    out << ToJson(profile).dump(2);
    if (!out) return Error{"io_error", "failed while writing profile"};
    return {};
}

Result<void> ProfileService::Remove(const std::string& name) const {
    std::error_code ec;
    fs::remove(FileFor(name), ec);
    if (ec) return Error{"io_error", ec.message()};
    return {};
}

Result<GameProfile> ProfileService::Import(const fs::path& file) const {
    std::ifstream in(file);
    if (!in) return Error{"io_error", "cannot open " + file.string()};
    nlohmann::json j;
    try { in >> j; }
    catch (const std::exception& e) { return Error{"parse_error", e.what()}; }
    std::string err;
    auto p = FromJson(j, &err);
    if (!p) return Error{"invalid_profile", err};
    return *p;
}

Result<void> ProfileService::Export(const GameProfile& profile, const fs::path& file) const {
    std::ofstream out(file, std::ios::trunc);
    if (!out) return Error{"io_error", "cannot open " + file.string()};
    out << ToJson(profile).dump(2);
    if (!out) return Error{"io_error", "failed while writing"};
    return {};
}

}
