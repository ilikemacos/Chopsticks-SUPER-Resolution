import { site } from "@/lib/site";

const btn =
  "rounded-xl border border-border px-6 py-3 font-medium text-fg transition-colors hover:border-primary/30";

export function Download() {
  return (
    <section id="download" className="mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="card-glass rounded-2xl p-8 sm:p-12">
        <p className="section-label">Get the app</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient">
          Download
        </h2>
        <p className="mt-3 max-w-2xl text-muted">
          The native desktop app is a real double-click .exe (.NET 8 / WPF): no
          install, no admin rights, runtime bundled. Built from source on Windows
          CI with a SHA-256 checksum.
        </p>

        <div className="mt-8 flex flex-wrap gap-3">
          <a
            href={site.appExeUrl}
            className="rounded-xl bg-primary px-6 py-3 font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
          >
            Download the app ({site.appVersion}, .exe)
          </a>
          {site.installersReady && (
            <>
              <a href={site.appMsiUrl} className={btn}>Installer (.msi)</a>
              <a href={site.appZipUrl} className={btn}>Portable (.zip)</a>
            </>
          )}
          <a href={site.releasesUrl} className={btn}>All releases</a>
          <a href={site.portableZipUrl} className={btn}>PowerShell edition (.zip)</a>
        </div>

        <p className="mt-4 max-w-2xl text-xs leading-relaxed text-faint">
          {site.installersReady
            ? "An .msi installer and a portable .zip bundling the CSR tools (ufx-upscale, ufx-live) are also available."
            : "An .msi installer and a portable .zip bundling the CSR tools (ufx-upscale, ufx-live) ship with the next release."}
        </p>
      </div>
    </section>
  );
}
