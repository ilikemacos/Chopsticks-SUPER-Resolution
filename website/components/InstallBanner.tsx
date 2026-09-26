import { site } from "@/lib/site";

export function InstallBanner() {
  const iex = `iex (iwr -useb ${site.installScriptUrl})`;
  return (
    <div className="mx-auto mt-6 max-w-3xl px-4 sm:px-6">
      <div className="card-glass rounded-xl border border-border p-4">
        <div className="flex flex-wrap items-center justify-between gap-3">
          <p className="font-mono text-[11px] uppercase tracking-widest text-muted">
            Install in one line (PowerShell, verifies checksums, no admin)
          </p>
          <a
            href={site.appExeUrl}
            className="rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
          >
            Download .exe ({site.appVersion})
          </a>
        </div>
        <pre className="mt-3 overflow-x-auto font-mono text-sm text-primary">
          <code>{iex}</code>
        </pre>
      </div>
    </div>
  );
}
