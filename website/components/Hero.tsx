import { site } from "@/lib/site";

export function Hero() {
  return (
    <section className="relative overflow-hidden bg-grid-fade">
      <div className="mx-auto max-w-6xl px-4 py-24 text-center sm:px-6 sm:py-32">
        <div className="mx-auto mb-6 inline-flex items-center gap-2 rounded-full border border-border bg-surface px-3 py-1 text-xs text-muted">
          <span className="h-1.5 w-1.5 rounded-full bg-good" />
          Open source · MIT · Windows 11
        </div>
        <h1 className="mx-auto max-w-3xl text-4xl font-semibold leading-tight tracking-tight sm:text-6xl">
          Universal graphics enhancement for Windows 11.
        </h1>
        <p className="mx-auto mt-4 max-w-xl text-lg text-muted">
          FSR. XeSS. Frame Generation. One interface.
        </p>
        <p className="mx-auto mt-3 max-w-2xl text-sm text-muted/80">
          A single place to configure the upscalers your games already support —
          and honest about what can and cannot be added from the outside.
        </p>
        <div className="mt-8 flex flex-wrap items-center justify-center gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-lg bg-accent px-6 py-3 font-medium text-white shadow-lg shadow-accent/20 transition-colors hover:bg-accentSoft"
          >
            Download for Windows (.exe)
          </a>
          <a
            href={site.githubUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            View on GitHub
          </a>
        </div>
        <p className="mt-4 text-xs text-muted/70">
          Windows 11 64-bit · x64 · native .NET 8 app, {site.appVersion} · no
          bundled proprietary vendor binaries ·{" "}
          <a href="#download" className="underline hover:text-white">
            other install options
          </a>
        </p>
      </div>
    </section>
  );
}
