const faqs = [
  {
    q: "Can Universal FrameFX add FSR or XeSS to any game?",
    a: "Not FSR 2/3/4 or XeSS, and any tool that claims to is misleading you: temporal upscalers need motion vectors, depth buffers and jitter offsets that only the game engine produces. The kind that can work on any window is spatial, because it operates on a finished frame — that is what CSR is, and it is the only upscaler here that needs no game support. To be clear about the current state: CSR itself is finished and tested, but applying it to a live window is not, so today the app runs it on an image you choose. Everything else Universal FrameFX does is configure the upscalers a game already ships, and — where a public, redistributable wrapper DLL exists — drop it into a specific game folder with a full backup.",
  },
  {
    q: "What is CSR, and is it as good as FSR 2 or DLSS?",
    a: "CSR (Chopsticks Super Resolution) is our own spatial upscaler, derived from AMD FSR 1's EASU + RCAS design and reimplemented from its MIT-licensed source: a 16-tap edge-adaptive resolve with a deringing clamp, then contrast-limited sharpening that adapts to local variance. Against the same references it measures +1.48 dB PSNR over FSR 1 and +1.90 dB over bilinear. It is not comparable to FSR 2 or DLSS: those reconstruct detail from previous frames, and no spatial filter can recover detail the game never rendered. Expect clearly better than a plain resample, not native quality. It also costs GPU time and creates no frames, so no FPS figure attaches to it.",
  },
  {
    q: "Why is CSR based on FSR 1 and not on XeSS?",
    a: "Because XeSS cannot be a starting point for this, and the reason is structural rather than a matter of effort. XeSS is a temporal, machine-learning upscaler: it needs motion vectors, depth and jitter from the renderer, and it needs trained network weights that ship as an Intel binary. None of that is available from outside a game's process, and bundling vendor binaries is something this project does not do. FSR 1 is the opposite on every count — spatial, pure arithmetic, no engine data, no weights, and published by AMD under the MIT licence, so it can be reimplemented with attribution. That is why CSR derives from FSR 1's EASU + RCAS. XeSS remains in the app as what it honestly is: a technology the game itself must ship, which we detect and configure.",
  },
  {
    q: "Does it work on my NVIDIA / Intel GPU?",
    a: "CSR runs on any GPU that can run a Direct3D 11 compute shader. FSR 1/2/3 and XeSS are cross-vendor and run on NVIDIA, AMD and Intel. FSR 4 requires AMD RDNA 4 hardware. GPU brand does not decide availability on its own — the app reports the real requirement for your exact configuration, and where it cannot confirm your GPU's architecture it says \"undetermined\" rather than guessing either way.",
  },
  {
    q: "Can it add frame generation to a game that doesn't have it?",
    a: "No. Frame generation intercepts the swapchain and needs engine motion vectors and UI composition data. There is no reliable external injection path. Where a game ships FSR 3 or XeSS frame generation, the app exposes the game's own toggle.",
  },
  {
    q: "Is it safe to use with online games?",
    a: "Do not use file modifications with anti-cheat-protected multiplayer titles. Universal FrameFX detects EasyAntiCheat, BattlEye and similar and refuses to write into those folders. Config changes to single-player games are backed up and reversible.",
  },
  {
    q: "Does it modify Windows system files?",
    a: "Never. It writes only inside a game's own folder (after a backup) and stores its own data under %LOCALAPPDATA%\\UniversalFrameFX. It does not touch system DLLs, Defender, or SmartScreen.",
  },
  {
    q: "Is it really open source?",
    a: "Yes — MIT licensed. No proprietary AMD/Intel/NVIDIA binaries are bundled. If a vendor SDK is needed, the app explains how to obtain it legally.",
  },
  {
    q: "Does it show real performance numbers?",
    a: "It never fabricates FPS. Where measurement is possible it uses Windows and GPU APIs and clearly distinguishes measured values from estimates. Resolution math (e.g. 1280×720 → 2560×1440) is a configuration estimate, labelled as such.",
  },
];

export function Faq() {
  return (
    <section id="faq" className="mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="mx-auto max-w-2xl text-center">
        <p className="section-label">Questions</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient sm:text-5xl">
          FAQ
        </h2>
      </div>
      <div className="card-glass mx-auto mt-10 max-w-3xl divide-y divide-border rounded-2xl">
        {faqs.map((f) => (
          <details key={f.q} className="group p-6">
            <summary className="cursor-pointer list-none font-display font-medium marker:content-none">
              <span className="flex items-center justify-between">
                {f.q}
                <span className="text-muted transition-transform group-open:rotate-45">
                  +
                </span>
              </span>
            </summary>
            <p className="mt-3 text-sm leading-relaxed text-muted">{f.a}</p>
          </details>
        ))}
      </div>
    </section>
  );
}
