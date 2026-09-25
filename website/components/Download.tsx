import { site } from "@/lib/site";

export function Download() {
  const iex = `iex (iwr -useb ${site.installScriptUrl})`;
  return (
    <section id="download" className="relative mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="orb animate-pulse-slow left-1/2 top-10 h-[360px] w-[360px] -translate-x-1/2 bg-primary/15" />
      <div className="card-glass relative z-10 rounded-2xl p-8 sm:p-12">
        <p className="section-label">Get the app</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient">
          Download
        </h2>
        <p className="mt-3 max-w-2xl text-muted">
          The native desktop app is a real double-click{" "}
          <code className="font-mono text-primary">.exe</code> — .NET 8 / WPF,
          dark modern UI, no install step and no admin rights. The .NET runtime is
          bundled, so nothing else is required.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-xl bg-primary px-6 py-3 font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
          >
            Download the app ({site.appVersion} · .exe)
          </a>
          {site.installersReady && (
            <>
              <a
                href={site.appMsiUrl}
                className="rounded-xl border border-border px-6 py-3 font-medium text-fg transition-colors hover:border-primary/30"
              >
                Installer (.msi)
              </a>
              <a
                href={site.appZipUrl}
                className="rounded-xl border border-border px-6 py-3 font-medium text-fg transition-colors hover:border-primary/30"
              >
                Portable (.zip)
              </a>
            </>
          )}
          <a
            href={site.releasesUrl}
            className="rounded-xl border border-border px-6 py-3 font-medium text-fg transition-colors hover:border-primary/30"
          >
            All releases &amp; checksums
          </a>
        </div>
        <p className="mt-3 text-xs leading-relaxed text-faint">
          The <code className="font-mono">.exe</code> is the self-contained WPF
          app (no install), built from source on Windows CI with a SHA-256
          checksum.
          {site.installersReady ? (
            <>
              {" "}The <code className="font-mono">.msi</code> installs
              per-machine into Program Files (needs admin). The{" "}
              <code className="font-mono">.zip</code> is the portable C++ build
              and bundles the CSR command-line tools —{" "}
              <code className="font-mono">ufx-upscale</code> (upscale a file) and{" "}
              <code className="font-mono">ufx-live</code> (the CSR real-time
              loop).
            </>
          ) : (
            <>
              {" "}An <code className="font-mono">.msi</code> installer and a
              portable <code className="font-mono">.zip</code> bundling the CSR
              command-line tools (<code className="font-mono">ufx-upscale</code>,{" "}
              <code className="font-mono">ufx-live</code>) ship with the next
              release.
            </>
          )}
        </p>

        <div className="mt-8 grid gap-4 text-sm text-muted sm:grid-cols-2">
          <div className="rounded-2xl border border-border bg-card/40 p-5">
            <p className="font-display font-semibold text-fg">Native app (.exe)</p>
            <p className="mt-1.5 leading-relaxed">
              Real GPU detection, GPU switching, an honest capability matrix for
              FSR / XeSS / frame generation, per-game profiles, and one-click
              auto-update. Self-contained — no runtime to install.
            </p>
          </div>
          <div className="rounded-2xl border border-border bg-card/40 p-5">
            <p className="font-display font-semibold text-fg">Prefer no download?</p>
            <p className="mt-1.5 leading-relaxed">
              A script-based PowerShell edition runs on any Windows 11 machine with
              nothing to install. See the alternative below.
            </p>
          </div>
        </div>

        <details className="mt-8 rounded-2xl border border-border bg-card/40 p-5">
          <summary className="cursor-pointer font-display text-sm font-semibold text-fg">
            Alternative: PowerShell edition (no .exe)
          </summary>
          <div className="mt-4 flex flex-wrap gap-3">
            <a
              href={site.portableZipUrl}
              className="rounded-lg border border-border px-5 py-2.5 text-sm font-medium text-fg transition-colors hover:border-primary/30"
            >
              Download .zip (portable)
            </a>
            <a
              href={site.installScriptUrl}
              className="rounded-lg border border-border px-5 py-2.5 text-sm font-medium text-fg transition-colors hover:border-primary/30"
            >
              View install.ps1
            </a>
          </div>
          <div className="mt-4 rounded-xl border border-border bg-card p-4">
            <p className="mb-2 font-mono text-[11px] uppercase tracking-widest text-muted">
              One-line install (verifies checksums, no admin)
            </p>
            <pre className="overflow-x-auto font-mono text-sm text-primary">
              <code>{iex}</code>
            </pre>
          </div>
        </details>

        <p className="mt-5 text-xs leading-relaxed text-faint">
          Nothing here disables Windows Defender or SmartScreen, uses hidden
          downloads, or requests elevation it does not need. Because the app is
          not code-signed, SmartScreen may prompt once — choose “More info → Run
          anyway”. Source, build and checksums are on GitHub.
        </p>
      </div>
    </section>
  );
}
