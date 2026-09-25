#include "LiveCommand.h"

#include "upscaling/CsrUpscaler.h"
#include "upscaling/QualityMode.h"
#include "upscaling/csr/ImageIo.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <limits>

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

// A gamma-encoded value; the scene is authored directly in perceptual space,
// which is what CSR's resolve expects.
inline void SetPixel(csr::Image& img, int x, int y, float r, float g, float b) {
    const size_t i = (static_cast<size_t>(y) * img.width + x) * 3;
    img.data[i + 0] = r;
    img.data[i + 1] = g;
    img.data[i + 2] = b;
}

}  // namespace

csr::Image MakeLiveFrame(int width, int height, uint32_t t) {
    csr::Image img(width, height);
    const float ft = static_cast<float>(t);

    // A slow horizontal scroll so every frame differs from the last, like a
    // panning camera. Kept integer-free so edges land on sub-pixel positions and
    // the resolve genuinely has to reconstruct them.
    const float scroll = ft * 1.7f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float u = (x + scroll);
            const float v = static_cast<float>(y);

            // 1. A hard diagonal edge that sweeps across the frame: a black/white
            //    boundary is the worst case for ringing, so it exercises the
            //    deringing clamp every frame.
            const float diag = std::sin((u + v) * 0.08f);
            float base = diag > 0.0f ? 0.92f : 0.08f;

            // 2. Mid-contrast vertical bars, where adaptive sharpening is allowed
            //    to act (unlike the full-contrast edge, where the limiter refuses).
            const float bars = 0.5f + 0.18f * std::sin(u * 0.5f);
            // Blend the bars into the bright side only, so both regimes coexist.
            if (base > 0.5f) base = bars;

            // 3. A moving bright block, to keep large flat regions in the mix.
            const int bx = static_cast<int>(scroll) % (width + 80) - 40;
            if (x >= bx && x < bx + 40 && y >= height / 3 && y < height / 3 + 40)
                base = 0.85f;

            // Give the channels slightly different content so the Rec.709 luma
            // direction (which only matters on colour) is exercised.
            SetPixel(img, x, y,
                     base,
                     base * 0.85f + 0.05f,
                     base * 0.70f + 0.10f);
        }
    }
    return img;
}

void PrintLiveUsage(std::ostream& out) {
    out <<
        "Universal FrameFX - CSR live upscale loop (CPU reference)\n"
        "\n"
        "Runs the real-time CSR pipeline - capture -> upscale -> present - over a\n"
        "stream of synthetic frames and measures the sustained per-frame cost.\n"
        "This is the cross-platform CPU counterpart of the GPU compute path in\n"
        "csr/src/Csr.Capture, which is the actual shipping real-time path and is\n"
        "far cheaper. Use this to prove the loop and to get an honest cost number.\n"
        "\n"
        "Usage:\n"
        "  ufx-live [options]\n"
        "\n"
        "Options:\n"
        "  --src <WxH>         Source (captured) resolution   (default 1280x720)\n"
        "  --quality <mode>    Ultra Quality | Quality | Balanced | Performance |\n"
        "                      Ultra Performance   (default: Quality = 1.5x)\n"
        "  --width <px>        Explicit output width  (overrides --quality)\n"
        "  --height <px>       Explicit output height (overrides --quality)\n"
        "  --frames <n>        Frames to push through the loop (default 120)\n"
        "  --sharpness <stops> Sharpening in stops; higher is weaker (default 0.25)\n"
        "  --no-dering         Disable the deringing clamp (not recommended)\n"
        "  --no-adaptive       Disable variance-adaptive sharpening\n"
        "  --save-last <png>   Write the final upscaled frame as proof\n"
        "  -q, --quiet         Only print the summary line\n"
        "  -h, --help          Show this help\n"
        "\n"
        "Example:\n"
        "  ufx-live --src 1280x720 --quality Quality --frames 120\n";
}

namespace {
bool ParseWxH(const std::string& s, uint32_t& w, uint32_t& h) {
    const auto pos = s.find_first_of("xX");
    if (pos == std::string::npos) return false;
    return ParseUint(s.substr(0, pos), w) && ParseUint(s.substr(pos + 1), h) &&
           w > 0 && h > 0;
}
}  // namespace

bool ParseLiveArgs(const std::vector<std::string>& argv, LiveArgs& args,
                   std::string& err, bool& showHelp) {
    showHelp = false;
    for (size_t i = 0; i < argv.size(); ++i) {
        const std::string& a = argv[i];
        auto next = [&](const char* name) -> const std::string* {
            if (i + 1 >= argv.size()) { err = std::string("missing value after ") + name; return nullptr; }
            return &argv[++i];
        };
        if (a == "-h" || a == "--help") { showHelp = true; return true; }
        else if (a == "--src") { auto* v = next("--src"); if (!v) return false;
                                 if (!ParseWxH(*v, args.srcWidth, args.srcHeight)) { err = "invalid --src (want WxH): " + *v; return false; } }
        else if (a == "--quality")   { auto* v = next("--quality");   if (!v) return false; args.quality = *v; }
        else if (a == "--width")     { auto* v = next("--width");     if (!v) return false;
                                       if (!ParseUint(*v, args.outWidth)) { err = "invalid --width: " + *v; return false; } }
        else if (a == "--height")    { auto* v = next("--height");    if (!v) return false;
                                       if (!ParseUint(*v, args.outHeight)) { err = "invalid --height: " + *v; return false; } }
        else if (a == "--frames")    { auto* v = next("--frames");    if (!v) return false;
                                       if (!ParseUint(*v, args.frames) || args.frames == 0) { err = "invalid --frames: " + *v; return false; } }
        else if (a == "--sharpness") { auto* v = next("--sharpness"); if (!v) return false;
                                       if (!ParseFloat(*v, args.sharpnessStops)) { err = "invalid --sharpness: " + *v; return false; } }
        else if (a == "--save-last") { auto* v = next("--save-last"); if (!v) return false; args.saveLastPath = *v; }
        else if (a == "--no-dering")   { args.dering = false; }
        else if (a == "--no-adaptive") { args.adaptiveSharpen = false; }
        else if (a == "-q" || a == "--quiet") { args.quiet = true; }
        else { err = "unknown option: " + a; return false; }
    }

    // Validate quality now so a typo fails before the loop runs.
    if (args.outWidth == 0 || args.outHeight == 0) {
        QualityMode qm{};
        if (!ParseQualityMode(args.quality, qm)) { err = "invalid --quality: " + args.quality; return false; }
    }
    return true;
}

namespace {
void ResolveOutputSize(const LiveArgs& args, uint32_t& ow, uint32_t& oh) {
    ow = args.outWidth;
    oh = args.outHeight;
    if (ow == 0 || oh == 0) {
        QualityMode qm{};
        ParseQualityMode(args.quality, qm);   // already validated
        const ScaleRatio r = FsrRatio(qm);
        ow = static_cast<uint32_t>(std::lround(args.srcWidth * r.x));
        oh = static_cast<uint32_t>(std::lround(args.srcHeight * r.y));
    }
}
}  // namespace

LiveStats RunLiveLoop(const LiveArgs& args,
                      const std::function<void(uint32_t, const csr::Image&)>& sink) {
    LiveStats s;
    s.srcW = args.srcWidth;
    s.srcH = args.srcHeight;
    ResolveOutputSize(args, s.outW, s.outH);
    s.minUpscaleMs = std::numeric_limits<double>::max();
    s.maxUpscaleMs = 0.0;

    csr::Options opt;
    opt.sharpnessStops = args.sharpnessStops;
    opt.dering = args.dering;
    opt.adaptiveSharpen = args.adaptiveSharpen;

    using clock = std::chrono::steady_clock;
    const auto wall0 = clock::now();

    for (uint32_t t = 0; t < args.frames; ++t) {
        // "Capture": a fresh frame arrives.
        const csr::Image frame =
            MakeLiveFrame(static_cast<int>(args.srcWidth), static_cast<int>(args.srcHeight), t);

        // "Upscale": the timed step, the same call the GPU path dispatches.
        const auto u0 = clock::now();
        const csr::Image up = CsrUpscaler::Run(frame, s.outW, s.outH, opt);
        const auto u1 = clock::now();

        const double ms = std::chrono::duration<double, std::milli>(u1 - u0).count();
        s.totalUpscaleMs += ms;
        if (ms < s.minUpscaleMs) s.minUpscaleMs = ms;
        if (ms > s.maxUpscaleMs) s.maxUpscaleMs = ms;
        s.framesProcessed++;

        // "Present": hand the finished frame to the sink.
        if (sink) sink(t, up);
    }

    const auto wall1 = clock::now();
    s.wallMs = std::chrono::duration<double, std::milli>(wall1 - wall0).count();
    if (s.framesProcessed == 0) s.minUpscaleMs = 0.0;
    return s;
}

int RunLive(const LiveArgs& args, std::ostream& out, std::ostream& err) {
    // Keep only the last frame around if we were asked to save it.
    csr::Image lastFrame;
    const bool wantSave = !args.saveLastPath.empty();

    std::function<void(uint32_t, const csr::Image&)> sink;
    if (wantSave) sink = [&](uint32_t, const csr::Image& up) { lastFrame = up; };

    if (!args.quiet) {
        out << "CSR live loop (CPU reference; the shipping real-time path is the GPU\n"
               "compute shader in csr/src/Csr.Capture and is far cheaper).\n";
    }

    const LiveStats s = RunLiveLoop(args, sink);

    if (s.outW == 0 || s.outH == 0) { err << "error: output size resolves to zero\n"; return 2; }

    if (wantSave) {
        std::string werr;
        if (!csr::SavePng(lastFrame, args.saveLastPath, werr)) {
            err << "error: " << werr << "\n";
            return 3;
        }
    }

    out << "  source        " << s.srcW << "x" << s.srcH << "\n"
        << "  output        " << s.outW << "x" << s.outH << "\n"
        << "  frames        " << s.framesProcessed << "\n"
        << "  upscale/frame " << s.MeanUpscaleMs() << " ms (min " << s.minUpscaleMs
        << ", max " << s.maxUpscaleMs << ")\n"
        << "  upscale rate  " << s.UpscaleFps() << " fps (upscale step alone)\n"
        << "  loop rate     " << s.WallFps() << " fps (generate + upscale + present)\n";
    if (wantSave) out << "  wrote         " << args.saveLastPath << "\n";

    return 0;
}

}  // namespace ufx
