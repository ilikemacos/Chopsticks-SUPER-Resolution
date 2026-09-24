import { site } from "@/lib/site";

export function Hero() {
  return (
    <section className="relative overflow-hidden">
      {/* Ambient glow orbs */}
      <div className="orb left-[-120px] top-[-160px] h-[440px] w-[440px] bg-accent/25" />
      <div className="orb right-[-140px] top-[40px] h-[420px] w-[420px] bg-good/10" />

      <div className="relative z-10 mx-auto max-w-5xl px-4 py-28 text-center sm:px-6 sm:py-36">
        <div className="mx-auto mb-7 inline-flex items-center gap-2 rounded-full border border-white/10 bg-white/[0.03] px-3.5 py-1.5 text-xs text-muted backdrop-blur">
          <span className="relative flex h-2 w-2">
            <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-good opacity-60" />
            <span className="relative inline-flex h-2 w-2 rounded-full bg-good" />
          </span>
          Open source · MIT · Windows 11
        </div>

        <h1 className="mx-auto max-w-4xl font-display text-5xl font-bold leading-[1.05] tracking-tight sm:text-7xl">
          <span className="text-gradient">Universal graphics</span>
          <br />
          <span className="text-gradient">enhancement.</span>
        </h1>

        <p className="mx-auto mt-6 max-w-xl text-lg text-muted">
          FSR. XeSS. Frame Generation. GPU switching. One interface — honest about
          what can and cannot be done.
        </p>

        <div className="mt-9 flex flex-wrap items-center justify-center gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-xl bg-accent px-7 py-3.5 font-medium text-white shadow-glow transition-colors hover:bg-accentSoft"
          >
            Download for Windows (.exe)
          </a>
          <a
            href={site.githubUrl}
            className="rounded-xl border border-border px-7 py-3.5 font-medium text-white transition-colors hover:border-accent"
          >
            View on GitHub
          </a>
        </div>

        <p className="mt-5 font-mono text-[11px] uppercase tracking-widest text-faint">
          Windows 11 64-bit · native .NET 8 app · {site.appVersion} ·{" "}
          <a href="#download" className="underline hover:text-muted">
            other install options
          </a>
        </p>
      </div>
    </section>
  );
}
