#include <gtest/gtest.h>

#include "profiles/GameScanner.h"

using namespace ufx;
namespace fs = std::filesystem;

TEST(Scanner, ParsesSteamLibraryFolders) {
    std::string vdf = R"(
"libraryfolders"
{
    "0"
    {
        "path"        "C:\\Program Files (x86)\\Steam"
    }
    "1"
    {
        "path"        "D:\\SteamLibrary"
    }
}
)";
    auto libs = GameScanner::ParseSteamLibraryFolders(vdf);
    ASSERT_EQ(libs.size(), 2u);
    EXPECT_EQ(libs[0].string(), "C:\\Program Files (x86)\\Steam");
    EXPECT_EQ(libs[1].string(), "D:\\SteamLibrary");
}

TEST(Scanner, ParsesAppManifest) {
    std::string acf = R"(
"AppState"
{
    "appid"        "1091500"
    "name"         "Cyberpunk 2077"
    "installdir"   "Cyberpunk 2077"
}
)";
    auto g = GameScanner::ParseSteamAppManifest(acf, "D:/SteamLibrary/steamapps");
    ASSERT_TRUE(g.has_value());
    EXPECT_EQ(g->name, "Cyberpunk 2077");
    EXPECT_EQ(g->source, "Steam");
    EXPECT_NE(g->installDir.string().find("Cyberpunk 2077"), std::string::npos);
}

TEST(Scanner, AppManifestMissingFieldsReturnsNullopt) {
    std::string acf = R"("AppState" { "appid" "1" })";
    EXPECT_FALSE(GameScanner::ParseSteamAppManifest(acf, "x").has_value());
}
