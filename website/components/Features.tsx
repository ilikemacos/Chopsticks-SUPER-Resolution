const features = [
  {
    title: "Real GPU detection",
    body: "NVIDIA, AMD and Intel — model, VRAM and driver read from WMI and the driver, and the DirectX feature level measured by actually creating a device rather than looking for DLLs on disk. When the architecture cannot be confirmed it says so instead of guessing.",
  },
  {
    title: "CSR — our own upscaler",
    body: "Chopsticks Super Resolution: a 16-tap edge-adaptive resolve with contrast-limited, variance-adaptive sharpening, derived from FSR 1's EASU + RCAS. It needs no engine data, so it is the one upscaler here that works on any app. Measured at +1.48 dB PSNR over FSR 1 and +1.90 dB over bilinear.",
  },
  {
    title: "Unified upscaling front-end",
    body: "CSR plus FSR 1/2/3, FSR 4 (RDNA 4 detected at runtime) and XeSS in one panel. Only the quality modes each implementation actually supports are shown, and anything that needs the game to ship it is labelled that way.",
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
    title: "Verified auto-update",
    body: "On launch it checks GitHub for a newer release and offers to download and relaunch it. The download's SHA-256 must match the checksum published beside it, or the file is deleted and never run.",
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
            className="card-glass rounded-2xl p-6 transition-colors"
          >
            <h3 className="font-display text-lg font-semibold">{f.title}</h3>
            <p className="mt-2 text-sm leading-relaxed text-muted">{f.body}</p>
          </div>
        ))}
      </div>
    </section>
  );
}
