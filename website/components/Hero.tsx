import Link from "next/link";
import { site } from "@/lib/site";
import { SystemCard } from "./SystemCard";
import { InstallBanner } from "./InstallBanner";

const tools = [
  { href: "/console#library", label: "Game Library", desc: "Installed titles" },
  { href: "/console#inspector", label: "Inspector", desc: "Per-game capabilities" },
  { href: "/console#profiles", label: "Profiles", desc: "Per-app configs" },
  { href: "/console#benchmarks", label: "Benchmarks", desc: "Measured FPS" },
  { href: "/console#settings", label: "Settings", desc: "Data & app" },
];

export function Hero() {
  return (
    <section className="relative overflow-hidden">
      <div className="orb animate-pulse-slow left-[-140px] top-[-180px] h-[460px] w-[460px] bg-primary/20 blur-[120px]" />
      <div className="orb animate-float right-[-160px] top-[20px] h-[440px] w-[440px] bg-primary/10" />

      <div className="relative z-10 mx-auto max-w-5xl px-4 pb-20 pt-24 text-center sm:px-6 sm:pt-32">
        <div className="mx-auto mb-8 inline-flex items-center gap-2.5 rounded-full border border-good/30 bg-good/5 px-4 py-1.5 text-xs text-muted backdrop-blur">
          <span className="relative flex h-2 w-2">
            <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-good opacity-60" />
            <span className="relative inline-flex h-2 w-2 rounded-full bg-good" />
          </span>
          <span className="font-mono uppercase tracking-widest">
            Open source / MIT / Windows 11
          </span>
        </div>

        <h1 className="mx-auto max-w-4xl font-display text-5xl font-bold leading-[1.02] tracking-tight sm:text-7xl">
          <span className="text-gradient">Universal upscaling</span>
          <br />
          <span className="text-gradient">utility.</span>
        </h1>

        <p className="mx-auto mt-6 max-w-2xl text-lg text-muted">
          CSR, our own spatial upscaler, plus real GPU detection, a game
          inspector, per-app profiles and measured benchmarks. Honest about what
          each technology can and cannot do, and about what is finished.
        </p>

        <div className="mt-9 flex flex-wrap items-center justify-center gap-3">
          <Link
            href="/console#scan"
            className="rounded-xl bg-primary px-7 py-3.5 font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
          >
            Scan my PC
          </Link>
          <Link
            href="/console#upscaling"
            className="rounded-xl border border-border px-7 py-3.5 font-medium text-fg transition-colors hover:border-primary/30"
          >
            Universal Upscaling
          </Link>
        </div>

        <InstallBanner />

        <p className="mt-5 font-mono text-[11px] uppercase tracking-[0.2em] text-faint">
          Windows 11 64-bit / native .NET 8 / {site.appVersion}
        </p>

        {/* Tool shortcuts */}
        <div className="mx-auto mt-14 grid max-w-3xl grid-cols-2 gap-3 sm:grid-cols-5">
          {tools.map((t) => (
            <Link
              key={t.href}
              href={t.href}
              className="card-glass rounded-2xl px-3 py-4 text-center transition-colors"
            >
              <span className="block font-display text-sm font-semibold text-fg">
                {t.label}
              </span>
              <span className="mt-0.5 block font-mono text-[10px] uppercase tracking-wider text-faint">
                {t.desc}
              </span>
            </Link>
          ))}
        </div>

        <SystemCard />
      </div>
    </section>
  );
}
