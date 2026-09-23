#pragma once

#include "core/Result.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ufx {

struct BackupRecord {
    std::string id;                 // timestamp-based
    std::string gameName;
    std::filesystem::path gameDir;
    std::vector<std::string> files; // relative paths backed up
    std::string createdUtc;
};

// Backs up and restores files within a *single game directory*. Never touches
// files outside the game folder, never touches Windows-owned files.
class BackupManager {
public:
    explicit BackupManager(std::filesystem::path backupRoot);

    // Copy the given files (relative to gameDir) into a new backup snapshot.
    Result<BackupRecord> Backup(const std::string& gameName,
                                const std::filesystem::path& gameDir,
                                const std::vector<std::string>& relativeFiles);

    // Restore a snapshot back into its original game directory.
    Result<void> Restore(const std::string& backupId);

    Result<std::vector<BackupRecord>> List() const;
    Result<void> Remove(const std::string& backupId);

    // Refuse to write into a game folder whose executable matches a known
    // anti-cheat signature. Returns the offending marker if found.
    static std::optional<std::string> DetectAntiCheat(const std::filesystem::path& gameDir);

private:
    std::filesystem::path root_;
    std::filesystem::path SnapshotDir(const std::string& id) const;
    std::filesystem::path MetaFile(const std::string& id) const;
};

}
