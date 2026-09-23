#include "AppPaths.h"

#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace ufx {

namespace fs = std::filesystem;

AppPaths AppPaths::resolve() {
    fs::path root;
#ifdef _WIN32
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        root = fs::path(path) / L"UniversalFrameFX";
        CoTaskMemFree(path);
    } else {
        root = fs::temp_directory_path() / "UniversalFrameFX";
    }
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    if (xdg && *xdg) {
        root = fs::path(xdg) / "UniversalFrameFX";
    } else if (home && *home) {
        root = fs::path(home) / ".config" / "UniversalFrameFX";
    } else {
        root = fs::temp_directory_path() / "UniversalFrameFX";
    }
#endif
    return AppPaths{
        root,
        root / "profiles",
        root / "logs",
        root / "backups",
        root / "cache",
    };
}

void AppPaths::ensure() const {
    std::error_code ec;
    for (const auto& p : {root, profiles, logs, backups, cache}) {
        fs::create_directories(p, ec);
    }
}

}
