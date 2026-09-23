import { site } from "@/lib/site";

export function Download() {
  const iex = `iex (iwr -useb ${site.installScriptUrl})`;
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <div className="rounded-2xl border border-border bg-surface p-8 sm:p-12">
        <h2 className="text-3xl font-semibold tracking-tight">Download</h2>
        <p className="mt-2 max-w-2xl text-muted">
          The runnable PowerShell edition works on any Windows 11 machine with no
          compiler and no admin rights. Downloads are served straight from this
          site.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.portableZipUrl}
            className="rounded-lg bg-accent px-6 py-3 font-medium text-white transition-colors hover:bg-accentSoft"
          >
            Download .zip (portable)
          </a>
          <a
            href={site.installScriptUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            View install.ps1
          </a>
        </div>

        <div className="mt-8 rounded-lg border border-border bg-bg p-4">
          <p className="mb-2 text-xs uppercase tracking-wide text-muted">
            One-line install (PowerShell — verifies checksums, no admin)
          </p>
          <pre className="overflow-x-auto text-sm text-accentSoft">
            <code>{iex}</code>
          </pre>
        </div>

        <div className="mt-6 grid gap-3 text-sm text-muted sm:grid-cols-2">
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">Portable .zip</p>
            <p className="mt-1">
              Extract and run{" "}
              <code className="text-accentSoft">UniversalFrameFX.cmd</code>. Uses
              PowerShell + .NET Windows Forms built into Windows 11 — real GPU
              detection, profiles, and backups, no installation.
            </p>
          </div>
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">One-line install</p>
            <p className="mt-1">
              The command above downloads and installs the app per-user, adds a
              Start Menu shortcut and an uninstall entry, and launches it.
            </p>
          </div>
        </div>

        <p className="mt-4 text-xs text-muted/80">
          Nothing here disables Windows Defender or SmartScreen, uses hidden
          downloads, or requests elevation it does not need. A native C++ build
          and MSI can also be produced from source — see the repository.
        </p>
      </div>
    </section>
  );
}
