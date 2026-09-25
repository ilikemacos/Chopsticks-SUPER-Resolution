#pragma once

// The CSR upscale command, shared between the console tool (ufx-upscale) and the
// main application's command-line mode. Kept out of the GUI so it is fully
// testable on any platform.

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace ufx {

struct UpscaleArgs {
    std::string inputPath;
    std::string outputPath;
    std::string quality = "Quality";   // a QualityMode name
    float sharpnessStops = 0.25f;
    bool dering = true;
    bool adaptiveSharpen = true;
    // Explicit output dimensions override the quality preset when both are > 0.
    uint32_t outWidth = 0;
    uint32_t outHeight = 0;
};

// Parses argv (excluding the program name). Returns true and fills `args`, or
// false and writes a message to `err`. `showHelp` is set when help was asked for.
bool ParseUpscaleArgs(const std::vector<std::string>& argv, UpscaleArgs& args,
                      std::string& err, bool& showHelp);

void PrintUpscaleUsage(std::ostream& out);

// Runs CSR against a real image file and writes the result. Returns a process
// exit code (0 == success); diagnostics go to `out` (progress) and `err`.
int RunUpscale(const UpscaleArgs& args, std::ostream& out, std::ostream& err);

}
