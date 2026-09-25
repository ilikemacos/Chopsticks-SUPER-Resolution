#include "UpscaleCommand.h"

#include "upscaling/CsrUpscaler.h"
#include "upscaling/QualityMode.h"
#include "upscaling/csr/CsrCore.h"
#include "upscaling/csr/ImageIo.h"

#include <charconv>
#include <cmath>

namespace ufx {
namespace {

bool ParseFloat(const std::string& s, float& out) {
    try { size_t idx = 0; out = std::stof(s, &idx); return idx == s.size(); }
    catch (...) { return false; }
}
bool ParseUint(const std::string& s, uint32_t& out) {
    uint32_t v = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc() || p != s.data() + s.size()) return false;
    out = v;
    return true;
}

}  // namespace

void PrintUpscaleUsage(std::ostream& out) {
    out <<
        "Universal FrameFX - CSR upscaler\n"
        "\n"
        "Upscales an image with CSR (Chopsticks Super Resolution), our own\n"
        "spatial upscaler derived from AMD FSR 1. Works on any image; needs\n"
        "nothing from a game engine.\n"
        "\n"
        "Usage:\n"
        "  ufx-upscale <input> <output.png> [options]\n"
        "\n"
        "Options:\n"
        "  --quality <mode>    Ultra Quality | Quality | Balanced | Performance |\n"
        "                      Ultra Performance   (default: Quality = 1.5x)\n"
        "  --width <px>        Explicit output width  (overrides --quality)\n"
        "  --height <px>       Explicit output height (overrides --quality)\n"
        "  --sharpness <stops> Sharpening in stops; higher is weaker (default 0.25)\n"
        "  --no-dering         Disable the deringing clamp (not recommended)\n"
        "  --no-adaptive       Disable variance-adaptive sharpening\n"
        "  -h, --help          Show this help\n"
        "\n"
        "Example:\n"
        "  ufx-upscale screenshot.png screenshot-2x.png --quality Performance\n";
}

bool ParseUpscaleArgs(const std::vector<std::string>& argv, UpscaleArgs& args,
                      std::string& err, bool& showHelp) {
    showHelp = false;
    std::vector<std::string> positionals;
    for (size_t i = 0; i < argv.size(); ++i) {
        const std::string& a = argv[i];
        auto next = [&](const char* name) -> const std::string* {
            if (i + 1 >= argv.size()) { err = std::string("missing value after ") + name; return nullptr; }
            return &argv[++i];
        };
        if (a == "-h" || a == "--help") { showHelp = true; return true; }
        else if (a == "--quality")   { auto* v = next("--quality");   if (!v) return false; args.quality = *v; }
        else if (a == "--sharpness") { auto* v = next("--sharpness"); if (!v) return false;
                                       if (!ParseFloat(*v, args.sharpnessStops)) { err = "invalid --sharpness: " + *v; return false; } }
        else if (a == "--width")     { auto* v = next("--width");     if (!v) return false;
                                       if (!ParseUint(*v, args.outWidth)) { err = "invalid --width: " + *v; return false; } }
        else if (a == "--height")    { auto* v = next("--height");    if (!v) return false;
                                       if (!ParseUint(*v, args.outHeight)) { err = "invalid --height: " + *v; return false; } }
        else if (a == "--no-dering")   { args.dering = false; }
        else if (a == "--no-adaptive") { args.adaptiveSharpen = false; }
        else if (!a.empty() && a[0] == '-') { err = "unknown option: " + a; return false; }
        else { positionals.push_back(a); }
    }
    if (positionals.size() != 2) {
        err = "expected exactly one input and one output path";
        return false;
    }
    args.inputPath = positionals[0];
    args.outputPath = positionals[1];

    // Validate quality now so a typo fails before we read the file.
    QualityMode qm{};
    if ((args.outWidth == 0 || args.outHeight == 0) && !ParseQualityMode(args.quality, qm)) {
        err = "invalid --quality: " + args.quality;
        return false;
    }
    return true;
}

int RunUpscale(const UpscaleArgs& args, std::ostream& out, std::ostream& err) {
    csr::LoadResult in = csr::LoadImage(args.inputPath);
    if (!in.ok) { err << "error: " << in.error << "\n"; return 2; }

    uint32_t ow = args.outWidth, oh = args.outHeight;
    if (ow == 0 || oh == 0) {
        QualityMode qm{};
        if (!ParseQualityMode(args.quality, qm)) { err << "error: invalid quality\n"; return 2; }
        const ScaleRatio r = FsrRatio(qm);
        ow = static_cast<uint32_t>(std::lround(in.image.width * r.x));
        oh = static_cast<uint32_t>(std::lround(in.image.height * r.y));
    }
    if (ow == 0 || oh == 0) { err << "error: output size resolves to zero\n"; return 2; }

    out << "CSR: " << in.image.width << "x" << in.image.height
        << " -> " << ow << "x" << oh << "\n";

    csr::Options opt;
    opt.sharpnessStops = args.sharpnessStops;
    opt.dering = args.dering;
    opt.adaptiveSharpen = args.adaptiveSharpen;

    const csr::Image result =
        CsrUpscaler::Run(in.image, ow, oh, opt);

    std::string werr;
    if (!csr::SavePng(result, args.outputPath, werr)) { err << "error: " << werr << "\n"; return 3; }

    out << "wrote " << args.outputPath << "\n";
    return 0;
}

}
