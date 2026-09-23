#include "GameScanner.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ufx {

namespace fs = std::filesystem;

std::vector<fs::path> GameScanner::ParseSteamLibraryFolders(const std::string& vdf) {
    // libraryfolders.vdf uses Valve KeyValues. We extract every "path" "<dir>".
    std::vector<fs::path> out;
    std::regex re("\"path\"\\s*\"([^\"]+)\"");
    for (std::sregex_iterator it(vdf.begin(), vdf.end(), re), end; it != end; ++it) {
        std::string p = (*it)[1].str();
        // VDF escapes backslashes; collapse "\\" to "\".
        std::string cleaned;
        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i] == '\\' && i + 1 < p.size() && p[i + 1] == '\\') { cleaned.push_back('\\'); ++i; }
            else cleaned.push_back(p[i]);
        }
        out.emplace_back(cleaned);
    }
    return out;
}

std::optional<DetectedGame> GameScanner::ParseSteamAppManifest(const std::string& acf,
                                                              const fs::path& steamApps) {
    std::smatch m;
    std::regex nameRe("\"name\"\\s*\"([^\"]+)\"");
    std::regex dirRe("\"installdir\"\\s*\"([^\"]+)\"");
    std::string name, installdir;
    if (std::regex_search(acf, m, nameRe)) name = m[1].str();
    if (std::regex_search(acf, m, dirRe))  installdir = m[1].str();
    if (name.empty() || installdir.empty()) return std::nullopt;

    DetectedGame g;
    g.name = name;
    g.installDir = steamApps / "common" / installdir;
    g.source = "Steam";
    return g;
}

std::vector<DetectedGame> GameScanner::ScanSteam() const {
    std::vector<DetectedGame> out;
#ifdef _WIN32
    // Steam install path from registry.
    HKEY key;
    fs::path steamPath;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH]; DWORD sz = sizeof(buf);
        if (RegQueryValueExW(key, L"SteamPath", nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(buf), &sz) == ERROR_SUCCESS) {
            steamPath = buf;
        }
        RegCloseKey(key);
    }
    if (steamPath.empty()) return out;

    std::vector<fs::path> libs = {steamPath / "steamapps"};
    fs::path libFile = steamPath / "steamapps" / "libraryfolders.vdf";
    std::ifstream in(libFile);
    if (in) {
        std::stringstream ss; ss << in.rdbuf();
        for (auto& lib : ParseSteamLibraryFolders(ss.str()))
            libs.push_back(lib / "steamapps");
    }

    std::error_code ec;
    for (const auto& steamApps : libs) {
        if (!fs::exists(steamApps, ec)) continue;
        for (const auto& e : fs::directory_iterator(steamApps, ec)) {
            if (ec) break;
            auto fn = e.path().filename().string();
            if (fn.rfind("appmanifest_", 0) != 0) continue;
            std::ifstream mf(e.path());
            if (!mf) continue;
            std::stringstream ss; ss << mf.rdbuf();
            if (auto g = ParseSteamAppManifest(ss.str(), steamApps)) out.push_back(*g);
        }
    }
#endif
    return out;
}

std::vector<DetectedGame> GameScanner::ScanEpic() const {
    std::vector<DetectedGame> out;
#ifdef _WIN32
    // Epic keeps per-app manifests as JSON in ProgramData.
    fs::path manifests = "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests";
    std::error_code ec;
    if (!fs::exists(manifests, ec)) return out;
    for (const auto& e : fs::directory_iterator(manifests, ec)) {
        if (ec) break;
        if (e.path().extension() != ".item") continue;
        std::ifstream in(e.path());
        if (!in) continue;
        nlohmann::json j;
        try { in >> j; } catch (...) { continue; }
        DetectedGame g;
        g.name = j.value("DisplayName", "");
        g.installDir = j.value("InstallLocation", "");
        std::string exe = j.value("LaunchExecutable", "");
        if (!g.installDir.empty() && !exe.empty()) g.executable = g.installDir / exe;
        g.source = "Epic";
        if (!g.name.empty()) out.push_back(g);
    }
#endif
    return out;
}

std::vector<DetectedGame> GameScanner::ScanGog() const {
    std::vector<DetectedGame> out;
#ifdef _WIN32
    // GOG Galaxy stores installed games under HKLM\SOFTWARE\WOW6432Node\GOG.com\Games.
    HKEY games;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\GOG.com\\Games",
                      0, KEY_READ, &games) != ERROR_SUCCESS)
        return out;
    wchar_t sub[256]; DWORD idx = 0, len = 256;
    while (RegEnumKeyExW(games, idx++, sub, &len, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        len = 256;
        HKEY g;
        if (RegOpenKeyExW(games, sub, 0, KEY_READ, &g) == ERROR_SUCCESS) {
            auto readStr = [&](const wchar_t* name) -> std::wstring {
                wchar_t buf[MAX_PATH]; DWORD sz = sizeof(buf);
                if (RegQueryValueExW(g, name, nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(buf), &sz) == ERROR_SUCCESS)
                    return buf;
                return {};
            };
            DetectedGame dg;
            std::wstring name = readStr(L"gameName");
            std::wstring path = readStr(L"path");
            dg.name = std::string(name.begin(), name.end());
            dg.installDir = path;
            dg.source = "GOG";
            if (!dg.name.empty()) out.push_back(dg);
            RegCloseKey(g);
        }
    }
    RegCloseKey(games);
#endif
    return out;
}

std::vector<DetectedGame> GameScanner::ScanXbox() const {
    // Xbox/Microsoft Store games live under WindowsApps with restrictive ACLs and
    // cannot generally be modified by user tools. We surface them as detected but
    // do not promise write access.
    std::vector<DetectedGame> out;
    return out;
}

std::vector<DetectedGame> GameScanner::ScanAll() const {
    std::vector<DetectedGame> out;
    using ScanFn = std::vector<DetectedGame> (GameScanner::*)() const;
    const ScanFn scanners[] = {
        &GameScanner::ScanSteam, &GameScanner::ScanEpic,
        &GameScanner::ScanGog, &GameScanner::ScanXbox,
    };
    for (ScanFn fn : scanners) {
        auto part = (this->*fn)();
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

}
