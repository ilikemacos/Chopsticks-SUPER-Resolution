import { site } from "@/lib/site";

const btn =
  "rounded-xl border border-border px-6 py-3 font-medium text-fg transition-colors hover:border-primary/30";

export function Download() {
  return (
    <section id="download" className="relative mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="orb animate-pulse-slow left-1/2 top-10 h-[360px] w-[360px] -translate-x-1/2 bg-primary/15" />
      <div className="card-glass relative z-10 rounded-2xl p-8 sm:p-12">
        <p className="section-label">Get the app</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient">
          Download
        </h2>
        <p className="mt-3 max-w-2xl text-muted">
          The native desktop app is a real double-click .exe (.NET 8 / WPF): no
          install step, no admin rights, .NET runtime bundled. Built from source
          on Windows CI with a SHA-256 checksum.
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
          <a href={site.releasesUrl} className={btn}>All releases &amp; checksums</a>
          <a href={site.portableZipUrl} className={btn}>PowerShell edition (.zip)</a>
        </div>

        <p className="mt-4 text-xs leading-relaxed text-faint">
          {site.installersReady
            ? "An .msi installer (per-machine, needs admin) and a portable .zip bundling the CSR command-line tools (ufx-upscale, ufx-live) are also available."
            : "An .msi installer and a portable .zip bundling the CSR command-line tools (ufx-upscale, ufx-live) ship with the next release."}{" "}
          Nothing here disables Defender or SmartScreen or requests elevation it
          does not need; the app is not code-signed, so SmartScreen may prompt
          once. Source, build and checksums are on GitHub.
        </p>
      </div>
    </section>
  );
}
