import { site } from "@/lib/site";

export function Download() {
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <div className="rounded-2xl border border-border bg-surface p-8 sm:p-12">
        <h2 className="text-3xl font-semibold tracking-tight">Download</h2>
        <p className="mt-2 max-w-2xl text-muted">
          The MSI is the standard installer. Every release ships a SHA-256
          checksum you can verify against the file.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.msiDownloadUrl}
            className="rounded-lg bg-accent px-6 py-3 font-medium text-white transition-colors hover:bg-accentSoft"
          >
            Download .msi (x64)
          </a>
          <a
            href={site.zipDownloadUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            Download .zip
          </a>
          <a
            href={site.releasesUrl}
            className="rounded-lg border border-border px-6 py-3 font-medium text-white transition-colors hover:border-accent"
          >
            All releases
          </a>
        </div>

        <div className="mt-6 grid gap-3 text-sm text-muted sm:grid-cols-2">
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">MSI installer (per-machine)</p>
            <p className="mt-1">
              Installs to Program Files with a Start Menu shortcut and an
              Add/Remove Programs entry. Windows asks for elevation only to write
              the shared install for all users.
            </p>
          </div>
          <div className="rounded-lg border border-border bg-bg p-4">
            <p className="font-medium text-white">Zip (portable)</p>
            <p className="mt-1">
              Verify the SHA-256, extract, and run
              {" "}
              <code className="text-accentSoft">UniversalFrameFX.exe</code>. No
              installation.
            </p>
          </div>
        </div>

        <div className="mt-4 rounded-lg border border-border bg-bg p-4">
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
          Installers never disable Windows Defender or SmartScreen, never use
          hidden downloads, and request no elevation they do not need. Direct
          download links resolve once a release has been published.
        </p>
      </div>
    </section>
  );
}
