#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ufx {

struct DetectedGame {
    std::string name;
    std::filesystem::path installDir;
    std::filesystem::path executable; // best-guess main executable, may be empty
    std::string source;               // "Steam", "Epic", "GOG", "Xbox", "Manual"
};

// Scans common PC launchers for installed games. All work is local; nothing is
// transmitted. On non-Windows the scanners return empty (used only for tests).
class GameScanner {
public:
    std::vector<DetectedGame> ScanAll() const;

    std::vector<DetectedGame> ScanSteam() const;
    std::vector<DetectedGame> ScanEpic() const;
    std::vector<DetectedGame> ScanGog() const;
    std::vector<DetectedGame> ScanXbox() const;

    // Parse a Steam libraryfolders.vdf payload into library paths. Exposed for tests.
    static std::vector<std::filesystem::path> ParseSteamLibraryFolders(const std::string& vdf);
    // Parse a Steam appmanifest_*.acf payload into a DetectedGame. Exposed for tests.
    static std::optional<DetectedGame> ParseSteamAppManifest(const std::string& acf,
                                                             const std::filesystem::path& steamApps);
};

}
