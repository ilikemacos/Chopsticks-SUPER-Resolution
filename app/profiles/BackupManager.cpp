#include "BackupManager.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>

namespace ufx {

namespace fs = std::filesystem;

namespace {

std::string NowStamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
    return buf;
}

}

BackupManager::BackupManager(fs::path backupRoot) : root_(std::move(backupRoot)) {
    std::error_code ec;
    fs::create_directories(root_, ec);
}

fs::path BackupManager::SnapshotDir(const std::string& id) const { return root_ / id / "files"; }
fs::path BackupManager::MetaFile(const std::string& id) const { return root_ / id / "backup.json"; }

std::optional<std::string> BackupManager::DetectAntiCheat(const fs::path& gameDir) {
    static const char* markers[] = {
        "EasyAntiCheat", "EasyAntiCheat_EOS", "BEService", "BattlEye",
        "beclient", "beclient_x64", "vgk.sys", "anticheat",
    };
    std::error_code ec;
    if (!fs::exists(gameDir, ec)) return std::nullopt;
    for (const auto& entry : fs::recursive_directory_iterator(
             gameDir, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) break;
        auto fname = entry.path().filename().string();
        for (const char* m : markers) {
            std::string lf = fname, lm = m;
            for (auto& ch : lf) ch = static_cast<char>(std::tolower((unsigned char)ch));
            for (auto& ch : lm) ch = static_cast<char>(std::tolower((unsigned char)ch));
            if (lf.find(lm) != std::string::npos) return std::string(m);
        }
    }
    return std::nullopt;
}

Result<BackupRecord> BackupManager::Backup(const std::string& gameName,
                                           const fs::path& gameDir,
                                           const std::vector<std::string>& relativeFiles) {
    std::error_code ec;
    if (!fs::exists(gameDir, ec))
        return Error{"not_found", "game directory does not exist: " + gameDir.string()};

    BackupRecord rec;
    rec.id = NowStamp();
    rec.gameName = gameName;
    rec.gameDir = gameDir;
    rec.createdUtc = rec.id;

    fs::create_directories(SnapshotDir(rec.id), ec);
    if (ec) return Error{"io_error", ec.message()};

    for (const auto& rel : relativeFiles) {
        fs::path src = gameDir / rel;
        if (!fs::exists(src, ec)) continue; // nothing to back up for a file we will create
        fs::path dst = SnapshotDir(rec.id) / rel;
        fs::create_directories(dst.parent_path(), ec);
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) return Error{"io_error", "copy failed for " + rel + ": " + ec.message()};
        rec.files.push_back(rel);
    }

    nlohmann::json meta;
    meta["id"] = rec.id;
    meta["gameName"] = rec.gameName;
    meta["gameDir"] = rec.gameDir.string();
    meta["files"] = rec.files;
    meta["createdUtc"] = rec.createdUtc;
    std::ofstream out(MetaFile(rec.id), std::ios::trunc);
    if (!out) return Error{"io_error", "cannot write backup metadata"};
    out << meta.dump(2);

    return rec;
}

Result<void> BackupManager::Restore(const std::string& backupId) {
    std::ifstream in(MetaFile(backupId));
    if (!in) return Error{"not_found", "backup not found: " + backupId};
    nlohmann::json meta;
    try { in >> meta; }
    catch (const std::exception& e) { return Error{"parse_error", e.what()}; }

    fs::path gameDir = meta.value("gameDir", "");
    if (gameDir.empty()) return Error{"invalid", "backup metadata missing gameDir"};

    std::error_code ec;
    for (const auto& rel : meta.value("files", std::vector<std::string>{})) {
        fs::path src = SnapshotDir(backupId) / rel;
        fs::path dst = gameDir / rel;
        if (!fs::exists(src, ec)) continue;
        fs::create_directories(dst.parent_path(), ec);
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (ec) return Error{"io_error", "restore failed for " + rel + ": " + ec.message()};
    }
    return {};
}

Result<std::vector<BackupRecord>> BackupManager::List() const {
    std::vector<BackupRecord> out;
    std::error_code ec;
    if (!fs::exists(root_, ec)) return out;
    for (const auto& entry : fs::directory_iterator(root_, ec)) {
        if (ec) break;
        auto meta = entry.path() / "backup.json";
        std::ifstream in(meta);
        if (!in) continue;
        nlohmann::json j;
        try { in >> j; } catch (...) { continue; }
        BackupRecord rec;
        rec.id = j.value("id", "");
        rec.gameName = j.value("gameName", "");
        rec.gameDir = j.value("gameDir", "");
        rec.files = j.value("files", std::vector<std::string>{});
        rec.createdUtc = j.value("createdUtc", "");
        out.push_back(std::move(rec));
    }
    return out;
}

Result<void> BackupManager::Remove(const std::string& backupId) {
    std::error_code ec;
    fs::remove_all(root_ / backupId, ec);
    if (ec) return Error{"io_error", ec.message()};
    return {};
}

}
