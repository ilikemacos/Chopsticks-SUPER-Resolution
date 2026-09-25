// ufx-live: a measured, cross-platform CSR real-time upscale loop.
//
// Shares its whole body with the GPU real-time path in csr/src/Csr.Capture
// (capture -> upscale -> present); here the source is synthetic and the upscale
// runs on the CPU core, so it runs anywhere and produces an honest per-frame
// cost. See LiveCommand.h for what it does and does not claim.

#include "LiveCommand.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

    ufx::LiveArgs parsed;
    std::string err;
    bool help = false;
    if (!ufx::ParseLiveArgs(args, parsed, err, help)) {
        std::cerr << "error: " << err << "\n\n";
        ufx::PrintLiveUsage(std::cerr);
        return 2;
    }
    if (help) { ufx::PrintLiveUsage(std::cout); return 0; }

    return ufx::RunLive(parsed, std::cout, std::cerr);
}
