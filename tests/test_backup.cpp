#include <gtest/gtest.h>

#include "profiles/BackupManager.h"

#include <cstdlib>
#include <atomic>
#include <fstream>
#include <random>
#include <stdexcept>

using namespace ufx;
namespace fs = std::filesystem;

namespace {
// A directory that is genuinely new on every call.
//
// This used to be temp_directory_path() / (tag + std::to_string(::rand())) with
// rand() never seeded, so each process run produced the *same* five names. The
// second `ctest` run on a machine then reused a directory that still held the
// previous run's snapshot, and Backup.ListReturnsSnapshots counted two where it
// expected one. CI never caught it because each CI runner starts with an empty
// temp directory; it only failed for anyone running the suite twice locally.
fs::path MakeTempDir(const std::string& tag) {
    static std::atomic<unsigned> counter{0};
    std::random_device rd;
    for (int attempt = 0; attempt < 64; ++attempt) {
        auto p = fs::temp_directory_path() /
                 (tag + std::to_string(rd()) + "_" + std::to_string(counter++));
        std::error_code ec;
        // create_directory (not create_directories) so an existing path is a
        // miss we retry rather than one we silently adopt.
        if (fs::create_directory(p, ec) && !ec) return p;
    }
    throw std::runtime_error("could not create a unique temp directory for " + tag);
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
