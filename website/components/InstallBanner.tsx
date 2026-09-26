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
    <div className="w-full border-b border-border bg-card/40 backdrop-blur">
      <div className="mx-auto flex max-w-6xl flex-wrap items-center gap-x-4 gap-y-2 px-4 py-2.5 sm:px-6">
        <span className="inline-flex items-center gap-2 font-mono text-[11px] uppercase tracking-widest text-muted">
          <span className="relative flex h-2 w-2">
            <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-good opacity-70" />
            <span className="relative inline-flex h-2 w-2 rounded-full bg-good" />
          </span>
          Install in one line
        </span>

        <div className="flex min-w-0 flex-1 items-center gap-2">
          <pre className="min-w-0 flex-1 overflow-x-auto rounded-lg border border-border bg-bg/60 px-3 py-1.5 font-mono text-sm text-primary">
            <code>{iex}</code>
          </pre>
          <button
            type="button"
            onClick={copy}
            aria-label="Copy install command"
            className="shrink-0 rounded-lg border border-border px-3 py-1.5 text-xs font-medium text-fg transition-colors hover:border-primary/30"
          >
            {copied ? "Copied" : "Copy"}
          </button>
        </div>

        <a
          href={site.appExeUrl}
          className="shrink-0 rounded-lg bg-primary px-4 py-1.5 text-sm font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
        >
          Download .exe ({site.appVersion})
        </a>
      </div>
    </div>
  );
}
