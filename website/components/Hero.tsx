import { site } from "@/lib/site";

const stats = [
  { value: "5", label: "Upscalers" },
  { value: "3", label: "GPU vendors" },
  { value: "$0", label: "MIT licensed" },
  { value: "0", label: "Fake FPS" },
];

export function Hero() {
  return (
    <section className="relative overflow-hidden">
      {/* Ambient floating glow orbs */}
      <div className="orb float-a left-[-140px] top-[-180px] h-[460px] w-[460px] bg-accent/25" />
      <div className="orb float-b right-[-160px] top-[20px] h-[440px] w-[440px] bg-good/10" />

      <div className="relative z-10 mx-auto max-w-5xl px-4 pb-24 pt-28 text-center sm:px-6 sm:pt-36">
        <div className="mx-auto mb-8 inline-flex items-center gap-2.5 rounded-full border border-white/10 bg-white/[0.03] px-4 py-1.5 text-xs text-muted backdrop-blur">
          <span className="relative flex h-2 w-2">
            <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-good opacity-60" />
            <span className="relative inline-flex h-2 w-2 rounded-full bg-good" />
          </span>
          <span className="font-mono uppercase tracking-widest">Open source · MIT · Windows 11</span>
        </div>

        <h1 className="mx-auto max-w-4xl font-display text-5xl font-bold leading-[1.02] tracking-tighter sm:text-[5.5rem]">
          <span className="text-gradient">Universal graphics</span>
          <br />
          <span className="text-gradient">enhancement</span>
        </h1>

        <p className="mx-auto mt-7 max-w-2xl text-lg text-muted sm:text-xl">
          FSR · XeSS · Frame Generation · GPU switching — one interface, honest
          about what can and cannot be done.
        </p>

        <div className="mt-10 flex flex-wrap items-center justify-center gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-xl bg-accent px-8 py-4 font-medium text-white shadow-glow transition-colors hover:bg-accentSoft"
          >
            Download for Windows (.exe)
          </a>
          <a
            href={site.githubUrl}
            className="rounded-xl border border-border px-8 py-4 font-medium text-white transition-colors hover:border-accent"
          >
            View on GitHub
          </a>
        </div>

        <p className="mt-6 font-mono text-[11px] uppercase tracking-[0.2em] text-faint">
          Windows 11 64-bit · native .NET 8 · {site.appVersion} ·{" "}
          <a href="#download" className="underline hover:text-muted">
            other install options
          </a>
        </p>

        {/* Stats strip */}
        <div className="mx-auto mt-16 grid max-w-3xl grid-cols-2 gap-px overflow-hidden rounded-2xl border border-white/8 bg-white/[0.03] sm:grid-cols-4">
          {stats.map((s) => (
            <div key={s.label} className="bg-bg/40 px-6 py-7 backdrop-blur">
              <div className="font-display text-3xl font-bold text-gradient">{s.value}</div>
              <div className="mt-1 font-mono text-[10px] uppercase tracking-widest text-faint">
                {s.label}
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
