const features = [
  {
    title: "Real GPU detection",
    body: "NVIDIA, AMD and Intel — model, VRAM, driver, DirectX feature level and Vulkan availability, read from DXGI and the driver, not guessed from the brand.",
  },
  {
    title: "Unified upscaling front-end",
    body: "FSR 1/2/3, FSR 4 (RDNA 4 detected at runtime) and XeSS in one panel. Only the quality modes each implementation actually supports are shown.",
  },
  {
    title: "Honest frame generation",
    body: "Clearly labels Supported, Requires game integration, or Requires a compatible implementation. It never pretends frame gen can be injected into any game.",
  },
  {
    title: "Per-game JSON profiles",
    body: "Executable, API, upscaler, quality, frame-gen, sharpening and FPS limit — all exportable and importable as plain JSON.",
  },
  {
    title: "Backup & one-click restore",
    body: "Before writing to a game folder, it snapshots the affected files. Restore reverts everything. It never touches files outside the game's own directory.",
  },
  {
    title: "Local diagnostics",
    body: "Export a redacted GPU/driver/API report as JSON or text. Nothing is uploaded automatically — your library never leaves your machine.",
  },
];

export function Features() {
  return (
    <section id="features" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <h2 className="text-3xl font-semibold tracking-tight">Features</h2>
      <p className="mt-2 max-w-2xl text-muted">
        A serious, transparent tool — not a marketing wrapper around fake FPS
        numbers.
      </p>
      <div className="mt-10 grid gap-5 sm:grid-cols-2 lg:grid-cols-3">
        {features.map((f) => (
          <div
            key={f.title}
            className="rounded-xl border border-border bg-surface p-6 transition-colors hover:border-accent/60"
          >
            <h3 className="text-lg font-medium">{f.title}</h3>
            <p className="mt-2 text-sm leading-relaxed text-muted">{f.body}</p>
          </div>
        ))}
      </div>
    </section>
  );
}
