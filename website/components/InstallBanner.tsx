"use client";

import { useState } from "react";
import { site } from "@/lib/site";

const iex = `iex (iwr -useb ${site.installScriptUrl})`;

export function InstallBanner() {
  const [copied, setCopied] = useState(false);

  async function copy() {
    try {
      await navigator.clipboard.writeText(iex);
      setCopied(true);
      setTimeout(() => setCopied(false), 1600);
    } catch {
      /* clipboard blocked; the text is selectable in the box */
    }
  }

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
        <div className="mt-3 flex items-center gap-2">
          <pre className="flex-1 overflow-x-auto rounded-lg border border-border bg-card px-3 py-2 font-mono text-sm text-primary">
            <code>{iex}</code>
          </pre>
          <button
            type="button"
            onClick={copy}
            aria-label="Copy install command"
            className="shrink-0 rounded-lg border border-border px-3 py-2 text-sm font-medium text-fg transition-colors hover:border-primary/30"
          >
            {copied ? "Copied" : "Copy"}
          </button>
        </div>
      </div>
    </div>
  );
}
