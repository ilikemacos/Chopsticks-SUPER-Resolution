"use client";

import { useRef } from "react";
import {
  Btn,
  Card,
  Empty,
  KeyVal,
  PanelHeading,
  Pill,
  downloadJson,
  readJsonFile,
} from "./ui";
import { profileSlug, methodById, type Profile } from "@/lib/engine";
import { site } from "@/lib/site";

// ---------------------------------------------------------------------------
// Shapes the desktop app exports. Documented here so both sides stay in sync.
// ---------------------------------------------------------------------------

export interface LibraryEntry {
  name: string;
  executablePath: string;
  installDir: string;
  source: string;
  sizeBytes?: number;
}

export interface InspectionReport {
  name: string;
  path: string;
  /** Graphics APIs evidenced on disk, e.g. ["DirectX 12", "Vulkan"]. */
  api: string[];
  nativeUpscalers: string[];
  frameGen: string[];
  latency: string[];
  hdr: boolean | null;
  /** Files that justified each conclusion — evidence, not inference. */
  evidence: string[];
  scannedAt: string;
}

export interface BenchmarkRun {
  label: string;
  method: string;
  quality: string;
  renderWidth: number;
  renderHeight: number;
  outputWidth: number;
  outputHeight: number;
  avgFps: number;
  onePercentLowFps: number;
  avgFrameTimeMs: number;
  durationSec: number;
}

export interface BenchmarkSet {
  game: string;
  capturedAt: string;
  capturedBy: string;
  runs: BenchmarkRun[];
}

function ImportButton({
  label,
  onData,
}: {
  label: string;
  onData: (data: unknown) => void;
}) {
  const ref = useRef<HTMLInputElement>(null);
  return (
    <>
      <input
        ref={ref}
        type="file"
        accept="application/json,.json"
        className="hidden"
        onChange={async (e) => {
          const file = e.target.files?.[0];
          if (!file) return;
          const data = await readJsonFile(file);
          if (data === null) {
            alert("That file is not valid JSON.");
          } else {
            onData(data);
          }
          e.target.value = "";
        }}
      />
      <Btn onClick={() => ref.current?.click()}>{label}</Btn>
    </>
  );
}

const desktopNote =
  "This data is produced by the Universal FrameFX desktop app, which can read the " +
  "filesystem, registry and driver. A web page cannot — so nothing here is " +
  "simulated: import a report and it renders exactly what was measured.";

// ---------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------

export function ProfilesPanel({
  profiles,
  onLoad,
  onDelete,
  onImport,
}: {
  profiles: Profile[];
  onLoad: (p: Profile) => void;
  onDelete: (name: string) => void;
  onImport: (data: unknown) => void;
}) {
  return (
    <div>
      <PanelHeading
        kicker="Per-application"
        title="Profiles"
        blurb="Saved configurations, stored in this browser. The JSON is the same schema the desktop app reads, so a profile exported here imports there unchanged."
      />

      <div className="mb-6 flex flex-wrap gap-3">
        <ImportButton label="Import profile JSON" onData={onImport} />
        <Btn
          onClick={() => downloadJson("framefx-profiles.json", profiles)}
          disabled={profiles.length === 0}
        >
          Export all ({profiles.length})
        </Btn>
      </div>

      {profiles.length === 0 ? (
        <Empty
          title="No profiles yet"
          body="Build one in Universal Upscaling and choose “Save as profile”, or import a JSON file you exported from the desktop app."
        />
      ) : (
        <div className="grid gap-4 md:grid-cols-2">
          {profiles.map((p) => {
            const m = methodById(p.method);
            return (
              <Card key={p.name}>
                <div className="mb-3 flex items-start justify-between gap-3">
                  <div className="min-w-0">
                    <p className="truncate font-display text-base font-semibold text-fg">
                      {p.name}
                    </p>
                    <p className="mt-0.5 truncate font-mono text-[11px] text-faint">
                      {p.executablePath || "no executable set"}
                    </p>
                  </div>
                  <Pill tone={p.mode === "external" ? "accent" : "muted"}>
                    {p.mode === "external" ? "External" : "Native"}
                  </Pill>
                </div>
                <KeyVal k="Method" v={m?.name ?? p.method} />
                <KeyVal k="Quality" v={p.quality} />
                <KeyVal
                  k="Resolution"
                  v={`${p.renderWidth}×${p.renderHeight} → ${p.outputWidth}×${p.outputHeight}`}
                />
                <KeyVal k="Sharpening" v={p.sharpness.toFixed(2)} />
                <KeyVal
                  k="Frame generation"
                  v={p.frameGenEnabled ? p.frameGen : "Off"}
                  tone={p.frameGenEnabled ? undefined : "muted"}
                />
                <KeyVal k="Safe mode" v={p.safeMode ? "On — no game files written" : "Off"} />
                <div className="mt-4 flex flex-wrap gap-2">
                  <Btn onClick={() => onLoad(p)}>Edit</Btn>
                  <Btn onClick={() => downloadJson(`${profileSlug(p.name)}.json`, p)}>Export</Btn>
                  <Btn variant="danger" onClick={() => onDelete(p.name)}>
                    Delete
                  </Btn>
                </div>
              </Card>
            );
          })}
        </div>
      )}
    </div>
  );
}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

export function LibraryPanel({
  library,
  setLibrary,
  onCreateProfile,
}: {
  library: LibraryEntry[];
  setLibrary: (v: LibraryEntry[]) => void;
  onCreateProfile: (e: LibraryEntry) => void;
}) {
  return (
    <div>
      <PanelHeading
        kicker="Installed titles"
        title="Game &amp; app library"
        blurb="The desktop app enumerates Steam, Epic, Xbox and Windows uninstall entries to build this list. Import that scan here to turn entries into profiles."
      />

      <div className="mb-6 flex flex-wrap gap-3">
        <ImportButton
          label="Import library scan"
          onData={(d) => {
            const arr = Array.isArray(d) ? d : (d as { games?: unknown }).games;
            if (!Array.isArray(arr)) return alert("Expected a library scan array.");
            const parsed = arr
              .filter((x): x is Record<string, unknown> => typeof x === "object" && x !== null)
              .map((x) => ({
                name: String(x.name ?? "Unknown"),
                executablePath: String(x.executablePath ?? ""),
                installDir: String(x.installDir ?? ""),
                source: String(x.source ?? "Imported"),
                sizeBytes: typeof x.sizeBytes === "number" ? x.sizeBytes : undefined,
              }));
            setLibrary(parsed);
          }}
        />
        {library.length > 0 ? (
          <Btn variant="danger" onClick={() => setLibrary([])}>
            Clear
          </Btn>
        ) : null}
      </div>

      {library.length === 0 ? (
        <Empty
          title="No library imported"
          body={`A browser cannot enumerate installed games — there is no filesystem or registry access. ${desktopNote}`}
          action={
            <Btn variant="primary" onClick={() => window.open(site.appExeUrl, "_self")}>
              Get the desktop app
            </Btn>
          }
        />
      ) : (
        <div className="card-glass overflow-hidden rounded-2xl">
          <table className="w-full text-sm">
            <thead>
              <tr className="border-b border-border bg-card/40 text-left">
                <th className="p-3.5 font-medium">Title</th>
                <th className="p-3.5 font-medium">Source</th>
                <th className="p-3.5 font-medium">Executable</th>
                <th className="p-3.5" />
              </tr>
            </thead>
            <tbody>
              {library.map((g) => (
                <tr key={`${g.name}-${g.executablePath}`} className="border-t border-border">
                  <td className="p-3.5 font-medium text-fg">{g.name}</td>
                  <td className="p-3.5">
                    <Pill>{g.source}</Pill>
                  </td>
                  <td className="max-w-[22rem] truncate p-3.5 font-mono text-[11px] text-faint">
                    {g.executablePath || g.installDir}
                  </td>
                  <td className="p-3.5 text-right">
                    <Btn onClick={() => onCreateProfile(g)}>Create profile</Btn>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

// ---------------------------------------------------------------------------
// Inspector
// ---------------------------------------------------------------------------

const inspectorTargets = [
  "DirectX 11 / 12 and Vulkan runtimes the title links against",
  "Native FSR 1/2/3/4 and XeSS SDK libraries shipped in the game folder",
  "Frame-generation modules (FSR 3 FG, XeSS FG)",
  "Reflex / latency libraries",
  "HDR support signalled by the engine or its config",
];

export function InspectorPanel({
  reports,
  setReports,
}: {
  reports: InspectionReport[];
  setReports: (v: InspectionReport[]) => void;
}) {
  return (
    <div>
      <PanelHeading
        kicker="Per-title capabilities"
        title="Game inspector"
        blurb="The app inspects a game folder and reports only what it can evidence on disk — the DLLs and runtimes actually present. It never guesses that a title supports a technology."
      />

      <div className="mb-6 flex flex-wrap gap-3">
        <ImportButton
          label="Import inspection report"
          onData={(d) => {
            const arr = Array.isArray(d) ? d : [d];
            const parsed = arr
              .filter((x): x is Record<string, unknown> => typeof x === "object" && x !== null)
              .map((x) => ({
                name: String(x.name ?? "Unknown"),
                path: String(x.path ?? ""),
                api: Array.isArray(x.api) ? x.api.map(String) : [],
                nativeUpscalers: Array.isArray(x.nativeUpscalers) ? x.nativeUpscalers.map(String) : [],
                frameGen: Array.isArray(x.frameGen) ? x.frameGen.map(String) : [],
                latency: Array.isArray(x.latency) ? x.latency.map(String) : [],
                hdr: typeof x.hdr === "boolean" ? x.hdr : null,
                evidence: Array.isArray(x.evidence) ? x.evidence.map(String) : [],
                scannedAt: String(x.scannedAt ?? ""),
              }));
            setReports(parsed);
          }}
        />
        {reports.length > 0 ? (
          <Btn variant="danger" onClick={() => setReports([])}>
            Clear
          </Btn>
        ) : null}
      </div>

      {reports.length === 0 ? (
        <Empty
          title="No inspection reports"
          body={`Inspection reads files inside a game's install folder, which a web page cannot do. ${desktopNote}`}
          action={
            <div className="text-left">
              <p className="mb-2 font-mono text-[10px] uppercase tracking-widest text-faint">
                What the inspector looks for
              </p>
              <ul className="space-y-1 text-xs text-muted">
                {inspectorTargets.map((t) => (
                  <li key={t}>• {t}</li>
                ))}
              </ul>
            </div>
          }
        />
      ) : (
        <div className="grid gap-4 md:grid-cols-2">
          {reports.map((r) => (
            <Card key={`${r.name}-${r.path}`}>
              <p className="font-display text-base font-semibold text-fg">{r.name}</p>
              <p className="mt-0.5 truncate font-mono text-[11px] text-faint">{r.path}</p>
              <div className="mt-4">
                <KeyVal k="Graphics APIs" v={r.api.length ? r.api.join(", ") : "none found"} tone={r.api.length ? undefined : "muted"} />
                <KeyVal
                  k="Native upscalers"
                  v={r.nativeUpscalers.length ? r.nativeUpscalers.join(", ") : "none found"}
                  tone={r.nativeUpscalers.length ? undefined : "muted"}
                />
                <KeyVal
                  k="Frame generation"
                  v={r.frameGen.length ? r.frameGen.join(", ") : "none found"}
                  tone={r.frameGen.length ? undefined : "muted"}
                />
                <KeyVal
                  k="Latency tech"
                  v={r.latency.length ? r.latency.join(", ") : "none found"}
                  tone={r.latency.length ? undefined : "muted"}
                />
                <KeyVal k="HDR" v={r.hdr === null ? "undetermined" : r.hdr ? "signalled" : "not signalled"} tone={r.hdr === null ? "muted" : undefined} />
              </div>
              {r.evidence.length > 0 ? (
                <details className="mt-4">
                  <summary className="cursor-pointer font-mono text-[10px] uppercase tracking-widest text-faint">
                    Evidence ({r.evidence.length})
                  </summary>
                  <ul className="mt-2 space-y-1 font-mono text-[11px] text-muted">
                    {r.evidence.map((e) => (
                      <li key={e} className="truncate">
                        {e}
                      </li>
                    ))}
                  </ul>
                </details>
              ) : null}
            </Card>
          ))}
        </div>
      )}
    </div>
  );
}

// ---------------------------------------------------------------------------
// Benchmarks
// ---------------------------------------------------------------------------

export function BenchmarksPanel({
  sets,
  setSets,
}: {
  sets: BenchmarkSet[];
  setSets: (v: BenchmarkSet[]) => void;
}) {
  return (
    <div>
      <PanelHeading
        kicker="Measured, never estimated"
        title="Benchmarks"
        blurb="Compares native rendering against each upscaling mode using frame times captured by the desktop app. Every number here comes from a real run — the site never generates performance figures."
      />

      <div className="mb-6 flex flex-wrap gap-3">
        <ImportButton
          label="Import benchmark results"
          onData={(d) => {
            const arr = Array.isArray(d) ? d : [d];
            const parsed = arr
              .filter((x): x is Record<string, unknown> => typeof x === "object" && x !== null)
              .map((x) => ({
                game: String(x.game ?? "Unknown"),
                capturedAt: String(x.capturedAt ?? ""),
                capturedBy: String(x.capturedBy ?? ""),
                runs: Array.isArray(x.runs)
                  ? x.runs
                      .filter((r): r is Record<string, unknown> => typeof r === "object" && r !== null)
                      .map((r) => ({
                        label: String(r.label ?? ""),
                        method: String(r.method ?? ""),
                        quality: String(r.quality ?? ""),
                        renderWidth: Number(r.renderWidth ?? 0),
                        renderHeight: Number(r.renderHeight ?? 0),
                        outputWidth: Number(r.outputWidth ?? 0),
                        outputHeight: Number(r.outputHeight ?? 0),
                        avgFps: Number(r.avgFps ?? 0),
                        onePercentLowFps: Number(r.onePercentLowFps ?? 0),
                        avgFrameTimeMs: Number(r.avgFrameTimeMs ?? 0),
                        durationSec: Number(r.durationSec ?? 0),
                      }))
                  : [],
              }));
            setSets(parsed);
          }}
        />
        {sets.length > 0 ? (
          <Btn variant="danger" onClick={() => setSets([])}>
            Clear
          </Btn>
        ) : null}
      </div>

      {sets.length === 0 ? (
        <Empty
          title="No benchmark data"
          body={`Frame times can only be captured from a running game by the desktop app. ${desktopNote}`}
        />
      ) : (
        <div className="space-y-6">
          {sets.map((s) => {
            const peak = Math.max(1, ...s.runs.map((r) => r.avgFps));
            const baseline = s.runs.find((r) => /native/i.test(r.label) || /native/i.test(r.quality));
            return (
              <Card key={`${s.game}-${s.capturedAt}`}>
                <div className="mb-5 flex flex-wrap items-center justify-between gap-3">
                  <div>
                    <p className="font-display text-lg font-semibold text-fg">{s.game}</p>
                    <p className="mt-0.5 font-mono text-[11px] text-faint">
                      {s.capturedAt ? new Date(s.capturedAt).toLocaleString() : "no timestamp"}
                      {s.capturedBy ? ` · ${s.capturedBy}` : ""}
                    </p>
                  </div>
                  <Pill tone="good">Measured</Pill>
                </div>

                <div className="space-y-3">
                  {s.runs.map((r) => {
                    const delta =
                      baseline && baseline.avgFps > 0 && r !== baseline
                        ? ((r.avgFps - baseline.avgFps) / baseline.avgFps) * 100
                        : null;
                    return (
                      <div key={r.label}>
                        <div className="mb-1 flex items-baseline justify-between gap-3 text-xs">
                          <span className="truncate text-fg">
                            {r.label}
                            <span className="ml-2 font-mono text-[10px] text-faint">
                              {r.renderWidth}×{r.renderHeight} → {r.outputWidth}×{r.outputHeight}
                            </span>
                          </span>
                          <span className="shrink-0 font-mono text-muted">
                            {r.avgFps.toFixed(1)} fps · 1% low {r.onePercentLowFps.toFixed(1)} ·{" "}
                            {r.avgFrameTimeMs.toFixed(2)} ms
                            {delta !== null ? (
                              <span className={delta >= 0 ? " text-good" : " text-bad"}>
                                {" "}
                                {delta >= 0 ? "+" : ""}
                                {delta.toFixed(0)}%
                              </span>
                            ) : null}
                          </span>
                        </div>
                        <div className="h-2 overflow-hidden rounded-full bg-mutedBg">
                          <div
                            className="h-full rounded-full bg-primary"
                            style={{ width: `${Math.max(2, (r.avgFps / peak) * 100)}%` }}
                          />
                        </div>
                      </div>
                    );
                  })}
                </div>
              </Card>
            );
          })}
        </div>
      )}
    </div>
  );
}
