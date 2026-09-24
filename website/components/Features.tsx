const features = [
  {
    title: "Real GPU detection",
    body: "NVIDIA, AMD and Intel — model, VRAM, driver, DirectX feature level and Vulkan availability, read from WMI and the driver, not guessed from the brand.",
  },
  {
    title: "Unified upscaling front-end",
    body: "FSR 1/2/3, FSR 4 (RDNA 4 detected at runtime) and XeSS in one panel. Only the quality modes each implementation actually supports are shown.",
  },
  {
    title: "GPU switching",
    body: "Pick which GPU a game launches on. Writes the per-app graphics preference Windows itself uses — high performance (discrete) or power saving (integrated). No driver hacks.",
  },
  {
    title: "Honest frame generation",
    body: "Clearly labels Supported, Requires game integration, or Requires a compatible implementation. It never pretends frame gen can be injected into any game.",
  },
  {
    title: "Per-game JSON profiles",
    body: "Executable, API, upscaler, quality, frame-gen, sharpening and output resolution — all exportable and importable as plain JSON.",
  },
  {
    title: "Backup & one-click restore",
    body: "Before writing to a game folder, it snapshots the affected files. Restore reverts everything. It refuses to touch anti-cheat-protected folders.",
  },
  {
    title: "One-click auto-update",
    body: "On launch it checks GitHub for a newer release and offers to download and relaunch it — no reinstall, no hunting for downloads.",
  },
  {
    title: "Local & private",
    body: "Everything runs on your machine. Your game library never leaves it, and no proprietary vendor binaries are bundled.",
  },
];

export function Features() {
  return (
    <section id="features" className="relative mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="mx-auto max-w-2xl text-center">
        <p className="section-label">What it does</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient sm:text-5xl">
          Features
        </h2>
        <p className="mx-auto mt-4 text-muted">
          A serious, transparent tool — not a marketing wrapper around fake FPS
          numbers.
        </p>
      </div>
      <div className="mt-12 grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
        {features.map((f) => (
          <div
            key={f.title}
            className="glass rounded-2xl p-6 transition-colors"
          >
            <h3 className="font-display text-lg font-semibold">{f.title}</h3>
            <p className="mt-2 text-sm leading-relaxed text-muted">{f.body}</p>
          </div>
        ))}
      </div>
    </section>
  );
}
