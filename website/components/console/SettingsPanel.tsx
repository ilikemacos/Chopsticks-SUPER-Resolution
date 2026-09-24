"use client";

import { Btn, Card, KeyVal, PanelHeading } from "./ui";
import { site } from "@/lib/site";

export function SettingsPanel({
  onExportAll,
  onReset,
}: {
  onExportAll: () => void;
  onReset: () => void;
}) {
  return (
    <div>
      <PanelHeading
        kicker="Preferences"
        title="Settings"
        blurb="Everything the console stores lives in this browser only. Nothing is uploaded, and there is no account."
      />
      <div className="grid gap-4 md:grid-cols-2">
        <Card>
          <p className="mb-3 font-display text-base font-semibold text-white">Your data</p>
          <KeyVal k="Storage" v="localStorage, this browser only" />
          <KeyVal k="Profile schema" v="v2 (desktop-app compatible)" />
          <KeyVal k="Telemetry" v="None" />
          <div className="mt-4 flex flex-wrap gap-2">
            <Btn onClick={onExportAll}>Export everything</Btn>
            <Btn variant="danger" onClick={onReset}>
              Clear local data
            </Btn>
          </div>
        </Card>
        <Card>
          <p className="mb-3 font-display text-base font-semibold text-white">Desktop app</p>
          <KeyVal k="Version" v={site.appVersion} />
          <KeyVal k="Detection" v="WMI + driver registry" />
          <KeyVal k="Safe mode" v="Config-only; never writes game files" />
          <div className="mt-4 flex flex-wrap gap-2">
            <a
              href={site.appExeUrl}
              className="inline-flex items-center rounded-lg bg-accent px-4 py-2 text-sm font-medium text-white shadow-glow transition-colors hover:bg-accentSoft"
            >
              Download {site.appVersion}
            </a>
            <a
              href={site.releasesUrl}
              className="inline-flex items-center rounded-lg border border-border px-4 py-2 text-sm font-medium text-white transition-colors hover:border-accent"
            >
              Release notes
            </a>
          </div>
        </Card>
      </div>

      <Card className="mt-4">
        <p className="mb-2 font-display text-base font-semibold text-white">Honesty policy</p>
        <ul className="space-y-1.5 text-xs leading-relaxed text-muted">
          <li>• Spatial upscaling is applied externally; temporal upscaling and frame generation are shown as native-only, because that is what the technology allows.</li>
          <li>• Hardware facts a browser cannot read are left blank rather than estimated.</li>
          <li>• Benchmark figures only ever come from measured runs imported from the desktop app.</li>
          <li>• Per-game capabilities come from files found on disk, never from a guess about the title.</li>
        </ul>
      </Card>
    </div>
  );
}
