#include <gtest/gtest.h>

#include "profiles/BackupManager.h"

#include <cstdlib>
#include <fstream>

using namespace ufx;
namespace fs = std::filesystem;

namespace {
fs::path MakeTempDir(const std::string& tag) {
    auto p = fs::temp_directory_path() / (tag + std::to_string(::rand()));
    fs::create_directories(p);
    return p;
}
void WriteFile(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::trunc);
    out << content;
}
std::string ReadFile(const fs::path& p) {
    std::ifstream in(p);
    std::stringstream ss; ss << in.rdbuf();
    return ss.str();
}
}

TEST(Backup, BackupThenRestoreRecoversOriginal) {
    auto gameDir = MakeTempDir("ufx_game_");
    auto backupRoot = MakeTempDir("ufx_backups_");
    WriteFile(gameDir / "engine.ini", "Upscaler=Off\n");

    BackupManager mgr(backupRoot);
    auto rec = mgr.Backup("Test Game", gameDir, {"engine.ini"});
    ASSERT_TRUE(rec.ok());
    EXPECT_EQ(rec.value().files.size(), 1u);

    // Simulate UFX editing the game file.
    WriteFile(gameDir / "engine.ini", "Upscaler=FSR3\n");
    EXPECT_EQ(ReadFile(gameDir / "engine.ini"), "Upscaler=FSR3\n");

    ASSERT_TRUE(mgr.Restore(rec.value().id).ok());
    EXPECT_EQ(ReadFile(gameDir / "engine.ini"), "Upscaler=Off\n");
}

TEST(Backup, ListReturnsSnapshots) {
    auto gameDir = MakeTempDir("ufx_game_");
    auto backupRoot = MakeTempDir("ufx_backups_");
    WriteFile(gameDir / "a.cfg", "x");

    BackupManager mgr(backupRoot);
    ASSERT_TRUE(mgr.Backup("G", gameDir, {"a.cfg"}).ok());
    auto list = mgr.List();
    ASSERT_TRUE(list.ok());
    EXPECT_EQ(list.value().size(), 1u);
    EXPECT_EQ(list.value()[0].gameName, "G");
}

TEST(Backup, MissingFileIsSkippedNotFatal) {
    auto gameDir = MakeTempDir("ufx_game_");
    auto backupRoot = MakeTempDir("ufx_backups_");

    BackupManager mgr(backupRoot);
    // File does not exist yet (UFX will create it) — backup should still succeed.
    auto rec = mgr.Backup("G", gameDir, {"willbecreated.dll"});
    ASSERT_TRUE(rec.ok());
    EXPECT_EQ(rec.value().files.size(), 0u);
}

TEST(Backup, DetectAntiCheatFindsMarker) {
    auto gameDir = MakeTempDir("ufx_ac_");
    WriteFile(gameDir / "EasyAntiCheat" / "EasyAntiCheat.exe", "stub");
    auto marker = BackupManager::DetectAntiCheat(gameDir);
    ASSERT_TRUE(marker.has_value());
}

TEST(Backup, DetectAntiCheatCleanFolder) {
    auto gameDir = MakeTempDir("ufx_clean_");
    WriteFile(gameDir / "game.exe", "stub");
    EXPECT_FALSE(BackupManager::DetectAntiCheat(gameDir).has_value());
}
