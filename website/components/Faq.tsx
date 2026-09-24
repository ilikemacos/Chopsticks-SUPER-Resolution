const faqs = [
  {
    q: "Can Universal FrameFX add FSR or XeSS to any game?",
    a: "No, and any tool that claims to is misleading you. Temporal upscalers need motion vectors, depth buffers and jitter offsets that only the game engine produces. Universal FrameFX configures the upscalers a game already ships, and — where a public, redistributable wrapper DLL exists — can drop it into a specific game folder with a full backup.",
  },
  {
    q: "Does it work on my NVIDIA / Intel GPU?",
    a: "FSR 1/2/3 and XeSS are cross-vendor and run on NVIDIA, AMD and Intel. FSR 4 requires AMD RDNA 4 hardware. GPU brand does not decide availability on its own — the app reports the real requirement for your exact configuration.",
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
      <div className="glass mx-auto mt-10 max-w-3xl divide-y divide-white/5 rounded-2xl">
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
