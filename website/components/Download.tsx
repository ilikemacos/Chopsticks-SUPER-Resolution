import { site } from "@/lib/site";

export function Download() {
  const iex = `iex (iwr -useb ${site.installScriptUrl})`;
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <div className="rounded-2xl border border-border bg-surface p-8 sm:p-12">
        <h2 className="text-3xl font-semibold tracking-tight">Download</h2>
        <p className="mt-2 max-w-2xl text-muted">
          The native desktop app is a real double-click{" "}
          <code className="text-accentSoft">.exe</code> — .NET 8 / WPF, dark
          modern UI, no install step and no admin rights. The .NET runtime is
          bundled, so nothing else is required.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-lg bg-accent px-6 py-3 font-medium text-white transition-colors hover:bg-accentSoft"
          >
            Download the app ({site.appVersion} · .exe)
          </a>
          <a
            href={site.releasesUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            All releases &amp; checksums
          </a>
        </div>

        <div className="mt-6 grid gap-3 text-sm text-muted sm:grid-cols-2">
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">Native app (.exe)</p>
            <p className="mt-1">
              Real GPU detection (WMI + driver VRAM), an honest capability matrix
              for FSR / XeSS / frame generation, per-game profiles, and game-folder
              backups. Self-contained — no runtime to install.
            </p>
          </div>
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">Prefer no download?</p>
            <p className="mt-1">
              A script-based PowerShell edition runs on any Windows 11 machine with
              nothing to install. See the alternative below.
            </p>
          </div>
        </div>

        <details className="mt-8 rounded-lg border border-border bg-bg p-4">
          <summary className="cursor-pointer text-sm font-medium text-white">
            Alternative: PowerShell edition (no .exe)
          </summary>
          <div className="mt-4 flex flex-wrap gap-3">
            <a
              href={site.portableZipUrl}
              className="rounded-lg border border-border px-5 py-2.5 text-sm font-medium text-white transition-colors hover:border-accent"
            >
              Download .zip (portable)
            </a>
            <a
              href={site.installScriptUrl}
              className="rounded-lg border border-border px-5 py-2.5 text-sm font-medium text-white transition-colors hover:border-accent"
            >
              View install.ps1
            </a>
          </div>
          <div className="mt-4 rounded-lg border border-border bg-surface p-4">
            <p className="mb-2 text-xs uppercase tracking-wide text-muted">
              One-line install (PowerShell — verifies checksums, no admin)
            </p>
            <pre className="overflow-x-auto text-sm text-accentSoft">
              <code>{iex}</code>
            </pre>
          </div>
          <p className="mt-3 text-xs text-muted/80">
            Uses PowerShell + .NET Windows Forms built into Windows 11 — same real
            GPU detection, profiles and backups, no installation.
          </p>
        </details>

        <p className="mt-4 text-xs text-muted/80">
          Nothing here disables Windows Defender or SmartScreen, uses hidden
          downloads, or requests elevation it does not need. Because the app is
          not code-signed, SmartScreen may prompt once — choose “More info →
          Run anyway”. Source, build and checksums are on GitHub.
        </p>
      </div>
    </section>
  );
}
