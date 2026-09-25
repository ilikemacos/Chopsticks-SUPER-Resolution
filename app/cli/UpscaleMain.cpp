// ufx-upscale: a standalone console front-end for the CSR upscaler.
//
// Cross-platform on purpose. The GUI application delegates to the same
// RunUpscale() when launched with command-line arguments, so "CSR upscales a
// real file" is one implementation, exercised by tests, not two.

#include "UpscaleCommand.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    if (args.empty()) { ufx::PrintUpscaleUsage(std::cout); return 1; }

    ufx::UpscaleArgs parsed;
    std::string err;
    bool help = false;
    if (!ufx::ParseUpscaleArgs(args, parsed, err, help)) {
        std::cerr << "error: " << err << "\n\n";
        ufx::PrintUpscaleUsage(std::cerr);
        return 2;
    }
    if (help) { ufx::PrintUpscaleUsage(std::cout); return 0; }

    return ufx::RunUpscale(parsed, std::cout, std::cerr);
}
