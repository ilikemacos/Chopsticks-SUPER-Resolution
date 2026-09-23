import { site } from "@/lib/site";

export function Download() {
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <div className="rounded-2xl border border-border bg-surface p-8 sm:p-12">
        <h2 className="text-3xl font-semibold tracking-tight">Download</h2>
        <p className="mt-2 max-w-2xl text-muted">
          Grab the latest signed release archive, or install from PowerShell.
          Every release ships a SHA-256 checksum the installer verifies.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.latestDownloadUrl}
            className="rounded-lg bg-accent px-6 py-3 font-medium text-white transition-colors hover:bg-accentSoft"
          >
            Download for Windows
          </a>
          <a
            href={site.releasesUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            All releases
          </a>
        </div>

        <div className="mt-8 rounded-lg border border-border bg-bg p-4">
          <p className="mb-2 text-xs uppercase tracking-wide text-muted">
            PowerShell install (verifies checksum, no admin required)
          </p>
          <pre className="overflow-x-auto text-sm text-accentSoft">
            <code>{`iex "& { $(iwr -useb ${site.githubUrl.replace(
              "https://github.com",
              "https://raw.githubusercontent.com"
            )}/main/installer/install.ps1) }"`}</code>
          </pre>
        </div>

        <p className="mt-4 text-xs text-muted/80">
          The installer never disables Windows Defender or SmartScreen, never
          uses hidden downloads, and requests no elevation it does not need.
        </p>
      </div>
    </section>
  );
}
