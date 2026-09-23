#pragma once

#include <filesystem>

namespace ufx {

// Locations under %LOCALAPPDATA%\UniversalFrameFX on Windows, and
// $XDG_CONFIG_HOME/UniversalFrameFX on other platforms (for tests / CI).
struct AppPaths {
    std::filesystem::path root;
    std::filesystem::path profiles;
    std::filesystem::path logs;
    std::filesystem::path backups;
    std::filesystem::path cache;

    static AppPaths resolve();
    void ensure() const;
};

}
