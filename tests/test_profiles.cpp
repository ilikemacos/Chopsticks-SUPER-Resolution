#include <gtest/gtest.h>

#include "profiles/ProfileService.h"

#include <cstdlib>
#include <filesystem>

using namespace ufx;
namespace fs = std::filesystem;

namespace {
AppPaths TempPaths() {
    auto base = fs::temp_directory_path() / ("ufx_test_" + std::to_string(::rand()));
    AppPaths p{ base, base / "profiles", base / "logs", base / "backups", base / "cache" };
    p.ensure();
    return p;
}
}

TEST(Profiles, RoundTripJson) {
    GameProfile p;
    p.name = "Cyber Example";
    p.executablePath = "C:/Games/Cyber/Cyber.exe";
    p.api = GraphicsApi::D3D12;
    p.upscaler = UpscalerId::Fsr3;
    p.quality = QualityMode::Quality;
    p.frameGen = FrameGenId::Fsr3Fg;
    p.frameGenEnabled = true;
    p.sharpness = 0.7f;
    p.outputWidth = 2560;
    p.outputHeight = 1440;
    p.fpsLimit = 120;

    auto j = ToJson(p);
    std::string err;
    auto back = FromJson(j, &err);
    ASSERT_TRUE(back.has_value()) << err;
    EXPECT_EQ(back->name, p.name);
    EXPECT_EQ(back->upscaler, UpscalerId::Fsr3);
    EXPECT_EQ(back->quality, QualityMode::Quality);
    EXPECT_EQ(back->frameGen, FrameGenId::Fsr3Fg);
    EXPECT_TRUE(back->frameGenEnabled);
    EXPECT_FLOAT_EQ(back->sharpness, 0.7f);
    ASSERT_TRUE(back->fpsLimit.has_value());
    EXPECT_EQ(*back->fpsLimit, 120);
}

TEST(Profiles, RejectsInvalidUpscaler) {
    nlohmann::json j;
    j["name"] = "Bad";
    j["upscaler"] = "FSR99";
    std::string err;
    auto back = FromJson(j, &err);
    EXPECT_FALSE(back.has_value());
    EXPECT_NE(err.find("upscaler"), std::string::npos);
}

TEST(Profiles, RejectsMissingName) {
    nlohmann::json j;
    j["upscaler"] = "FSR3";
    std::string err;
    EXPECT_FALSE(FromJson(j, &err).has_value());
}

TEST(Profiles, RejectsOutOfRangeSharpness) {
    nlohmann::json j;
    j["name"] = "X";
    j["sharpness"] = 5.0;
    std::string err;
    EXPECT_FALSE(FromJson(j, &err).has_value());
}

TEST(Profiles, SaveLoadRemove) {
    auto paths = TempPaths();
    ProfileService svc(paths);

    GameProfile p;
    p.name = "My Game";
    p.upscaler = UpscalerId::XeSS;
    ASSERT_TRUE(svc.Save(p).ok());

    auto all = svc.LoadAll();
    ASSERT_TRUE(all.ok());
    ASSERT_EQ(all.value().size(), 1u);
    EXPECT_EQ(all.value()[0].upscaler, UpscalerId::XeSS);

    ASSERT_TRUE(svc.Remove("My Game").ok());
    auto after = svc.LoadAll();
    ASSERT_TRUE(after.ok());
    EXPECT_EQ(after.value().size(), 0u);
}

TEST(Profiles, ImportExport) {
    auto paths = TempPaths();
    ProfileService svc(paths);
    GameProfile p;
    p.name = "Export Me";
    p.upscaler = UpscalerId::Fsr2;

    auto file = paths.cache / "exported.json";
    ASSERT_TRUE(svc.Export(p, file).ok());
    auto imported = svc.Import(file);
    ASSERT_TRUE(imported.ok());
    EXPECT_EQ(imported.value().name, "Export Me");
    EXPECT_EQ(imported.value().upscaler, UpscalerId::Fsr2);
}

TEST(Profiles, SlugIsFilesystemSafe) {
    EXPECT_EQ(ProfileService::Slug("Cyber Example 2077!"), "cyber-example-2077");
    EXPECT_EQ(ProfileService::Slug(""), "profile");
}

#ifdef UFX_EXAMPLES_DIR
TEST(Profiles, ShippedExamplesAllParse) {
    auto paths = TempPaths();
    ProfileService svc(paths);
    fs::path examples = UFX_EXAMPLES_DIR;
    ASSERT_TRUE(fs::exists(examples)) << "examples dir missing: " << examples;

    int count = 0;
    for (const auto& e : fs::directory_iterator(examples)) {
        if (e.path().extension() != ".json") continue;
        auto res = svc.Import(e.path());
        ASSERT_TRUE(res.ok()) << e.path().filename().string()
                              << ": " << (res.ok() ? "" : res.error().message);
        EXPECT_FALSE(res.value().name.empty());
        ++count;
    }
    EXPECT_GE(count, 3) << "expected at least three example profiles to ship";
}
#endif
