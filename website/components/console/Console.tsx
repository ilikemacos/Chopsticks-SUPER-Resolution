"use client";

import { useCallback, useEffect, useMemo, useState } from "react";
import {
  Btn,
  Card,
  Empty,
  Field,
  KeyVal,
  PanelHeading,
  Pill,
  Select,
  Slider,
  TextInput,
  downloadJson,
} from "./ui";
import {
  BenchmarksPanel,
  InspectorPanel,
  LibraryPanel,
  ProfilesPanel,
  type BenchmarkSet,
  type InspectionReport,
  type LibraryEntry,
} from "./DataPanels";
import {
  BROWSER_BLIND_SPOTS,
  detectSystem,
  type DetectedSystem,
} from "@/lib/detect";
import {
  COMMON_RESOLUTIONS,
  METHODS,
  QUALITY_PRESETS,
  emptyProfile,
  integrationLabel,
  methodById,
  parseProfile,
  planResolution,
  presetById,
  resolveMethods,
  type Profile,
} from "@/lib/engine";
import { site } from "@/lib/site";

const TABS = [
  { id: "scan", label: "Scan" },
  { id: "upscaling", label: "Universal Upscaling" },
  { id: "library", label: "Library" },
  { id: "inspector", label: "Inspector" },
  { id: "profiles", label: "Profiles" },
  { id: "benchmarks", label: "Benchmarks" },
  { id: "settings", label: "Settings" },
] as const;

type TabId = (typeof TABS)[number]["id"];

function load<T>(key: string, fallback: T): T {
  try {
    const raw = localStorage.getItem(`framefx.${key}`);
    return raw ? (JSON.parse(raw) as T) : fallback;
  } catch {
    return fallback;
  }
}

function save(key: string, value: unknown) {
  try {
    localStorage.setItem(`framefx.${key}`, JSON.stringify(value));
  } catch {
    /* private mode / quota — state simply does not persist */
  }
}

export function Console() {
  const [tab, setTab] = useState<TabId>("scan");
  const [system, setSystem] = useState<DetectedSystem | null>(null);
  const [scanning, setScanning] = useState(false);
  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [library, setLibraryState] = useState<LibraryEntry[]>([]);
  const [reports, setReportsState] = useState<InspectionReport[]>([]);
  const [sets, setSetsState] = useState<BenchmarkSet[]>([]);
  const [draft, setDraft] = useState<Profile>(() => emptyProfile());
  const [hydrated, setHydrated] = useState(false);

  // Hydrate from localStorage after mount so SSR output stays stable.
  useEffect(() => {
    setSystem(load<DetectedSystem | null>("system", null));
    setProfiles(load<Profile[]>("profiles", []));
    setLibraryState(load<LibraryEntry[]>("library", []));
    setReportsState(load<InspectionReport[]>("reports", []));
    setSetsState(load<BenchmarkSet[]>("benchmarks", []));
    const stored = load<Profile | null>("draft", null);
    if (stored) setDraft(stored);
    const hash = window.location.hash.replace("#", "");
    if (TABS.some((t) => t.id === hash)) setTab(hash as TabId);
    setHydrated(true);
  }, []);

  useEffect(() => {
    if (hydrated) save("draft", draft);
  }, [draft, hydrated]);

  const setLibrary = (v: LibraryEntry[]) => { setLibraryState(v); save("library", v); };
  const setReports = (v: InspectionReport[]) => { setReportsState(v); save("reports", v); };
  const setSets = (v: BenchmarkSet[]) => { setSetsState(v); save("benchmarks", v); };

  const go = useCallback((id: TabId) => {
    setTab(id);
    if (typeof window !== "undefined") window.history.replaceState(null, "", `#${id}`);
  }, []);

  const runScan = async () => {
    setScanning(true);
    try {
      const result = await detectSystem();
      setSystem(result);
      save("system", result);
      // Default the output resolution to the detected display.
      if (result.display.width > 0) {
        setDraft((d) => {
          const w = Math.round(result.display.width * result.display.dpr);
          const h = Math.round(result.display.height * result.display.dpr);
          const plan = planResolution(w, h, presetById(d.quality).ratio);
          return { ...d, outputWidth: w, outputHeight: h, renderWidth: plan.renderWidth, renderHeight: plan.renderHeight };
        });
      }
    } finally {
      setScanning(false);
    }
  };

  const availability = useMemo(() => resolveMethods(system?.gpu ?? null), [system]);

  const persistProfiles = (next: Profile[]) => {
    setProfiles(next);
    save("profiles", next);
  };

  const saveDraft = () => {
    if (!draft.name.trim()) return alert("Give the profile a name first.");
    const next = [...profiles.filter((p) => p.name !== draft.name), draft].sort((a, b) =>
      a.name.localeCompare(b.name),
    );
    persistProfiles(next);
    go("profiles");
  };

  return (
    <div className="mx-auto max-w-6xl px-4 pb-24 pt-10 sm:px-6">
      {/* Tab bar */}
      <div className="glass sticky top-[62px] z-30 mb-10 flex gap-1 overflow-x-auto rounded-xl p-1.5">
        {TABS.map((t) => (
          <button
            key={t.id}
            onClick={() => go(t.id)}
            className={`shrink-0 rounded-lg px-4 py-2 text-sm transition-colors ${
              tab === t.id ? "bg-accent text-white" : "text-muted hover:bg-white/5 hover:text-white"
            }`}
          >
            {t.label}
          </button>
        ))}
      </div>

      {tab === "scan" ? (
        <ScanPanel system={system} scanning={scanning} onScan={runScan} availability={availability} />
      ) : null}

      {tab === "upscaling" ? (
        <UpscalingPanel
          draft={draft}
          setDraft={setDraft}
          system={system}
          availability={availability}
          onSave={saveDraft}
        />
      ) : null}

      {tab === "library" ? (
        <LibraryPanel
          library={library}
          setLibrary={setLibrary}
          onCreateProfile={(g) => {
            setDraft((d) => ({ ...d, name: g.name, executablePath: g.executablePath }));
            go("upscaling");
          }}
        />
      ) : null}

      {tab === "inspector" ? <InspectorPanel reports={reports} setReports={setReports} /> : null}

      {tab === "profiles" ? (
        <ProfilesPanel
          profiles={profiles}
          onLoad={(p) => {
            setDraft(p);
            go("upscaling");
          }}
          onDelete={(name) => persistProfiles(profiles.filter((p) => p.name !== name))}
          onImport={(data) => {
            const arr = Array.isArray(data) ? data : [data];
            const parsed = arr.map(parseProfile).filter((p): p is Profile => p !== null);
            if (parsed.length === 0) return alert("No valid profiles in that file.");
            const merged = [...profiles];
            for (const p of parsed) {
              const i = merged.findIndex((x) => x.name === p.name);
              if (i >= 0) merged[i] = p;
              else merged.push(p);
            }
            persistProfiles(merged.sort((a, b) => a.name.localeCompare(b.name)));
          }}
        />
      ) : null}

      {tab === "benchmarks" ? <BenchmarksPanel sets={sets} setSets={setSets} /> : null}

      {tab === "settings" ? (
        <SettingsPanel
          onExportAll={() =>
            downloadJson("framefx-data.json", { system, profiles, library, reports, benchmarks: sets })
          }
          onReset={() => {
            if (!confirm("Clear all locally stored FrameFX data in this browser?")) return;
            ["system", "profiles", "library", "reports", "benchmarks", "draft"].forEach((k) => {
              try { localStorage.removeItem(`framefx.${k}`); } catch { /* ignore */ }
            });
            setSystem(null);
            setProfiles([]);
            setLibraryState([]);
            setReportsState([]);
            setSetsState([]);
            setDraft(emptyProfile());
          }}
        />
      ) : null}
    </div>
  );
}

// ---------------------------------------------------------------------------

function ScanPanel({
  system,
  scanning,
  onScan,
  availability,
}: {
  system: DetectedSystem | null;
  scanning: boolean;
  onScan: () => void;
  availability: ReturnType<typeof resolveMethods>;
}) {
  const d = system?.display;
  return (
    <div>
      <PanelHeading
        kicker="System detection"
        title="Scan my PC"
        blurb="Reads everything a browser genuinely exposes — GPU name, WebGL/WebGPU support, CPU threads, display mode, measured refresh rate and real HDR/colour-gamut capability. Anything a browser cannot see is listed as such, never estimated."
      />

      <div className="mb-8 flex flex-wrap items-center gap-3">
        <Btn variant="primary" onClick={onScan} disabled={scanning}>
          {scanning ? "Scanning…" : system ? "Re-scan" : "Scan my PC"}
        </Btn>
        {system ? (
          <span className="font-mono text-[11px] text-faint">
            Last scan {new Date(system.detectedAt).toLocaleString()}
          </span>
        ) : null}
      </div>

      {!system ? (
        <Empty
          title="No scan yet"
          body="Nothing is collected until you press Scan. Detection runs entirely in your browser and no data leaves this page."
        />
      ) : (
        <div className="space-y-6">
          <div className="grid gap-4 md:grid-cols-2">
            <Card>
              <div className="mb-3 flex items-center justify-between gap-3">
                <p className="font-display text-base font-semibold text-white">Graphics</p>
                <Pill tone={system.gpu ? "good" : "warn"}>
                  {system.gpu ? "Detected" : "Masked"}
                </Pill>
              </div>
              <KeyVal k="Reported GPU" v={system.glRenderer ?? "unavailable"} tone={system.glRenderer ? undefined : "muted"} />
              <KeyVal k="Vendor" v={system.gpu?.vendor ?? "Unknown"} />
              <KeyVal
                k="Architecture"
                v={system.gpu ? (system.gpu.uncertain ? `${system.gpu.arch} (uncertain)` : system.gpu.arch) : "Unknown"}
                tone={system.gpu?.uncertain ? "muted" : undefined}
              />
              <KeyVal k="WebGL 2" v={system.webgl2 ? "Supported" : "Not supported"} />
              <KeyVal k="WebGPU" v={system.webgpu ? system.webgpuInfo ?? "Supported" : "Not supported"} />
              <p className="mt-3 text-[11px] leading-relaxed text-faint">
                The GPU name comes from your browser and can be masked or virtualised.
                Architecture is inferred from that name — it is not read from the driver.
              </p>
            </Card>

            <Card>
              <p className="mb-3 font-display text-base font-semibold text-white">System &amp; display</p>
              <KeyVal k="Platform" v={system.platform} />
              <KeyVal k="CPU threads" v={system.cpuThreads ?? "unavailable"} tone={system.cpuThreads ? undefined : "muted"} />
              <KeyVal
                k="System memory"
                v={system.deviceMemoryGb ? `≥ ${system.deviceMemoryGb} GB (coarse)` : "unavailable"}
                tone={system.deviceMemoryGb ? undefined : "muted"}
              />
              <KeyVal k="Display" v={d ? `${d.width} × ${d.height} @ ${d.dpr}× DPR` : "—"} />
              <KeyVal k="Refresh rate" v={d?.refreshHz ? `${d.refreshHz} Hz (measured)` : "not measured"} tone={d?.refreshHz ? undefined : "muted"} />
              <KeyVal k="HDR" v={d?.hdr === null || d?.hdr === undefined ? "undetermined" : d.hdr ? "High dynamic range" : "Standard"} tone={d?.hdr ? undefined : "muted"} />
              <KeyVal k="Colour gamut" v={d?.colorGamut ?? "Unknown"} />
            </Card>
          </div>

          <Card>
            <p className="mb-1 font-display text-base font-semibold text-white">
              What your hardware can run
            </p>
            <p className="mb-4 text-xs text-muted">
              Hardware gates only. Whether a specific game ships a technology is a per-title
              fact — use the Inspector for that.
            </p>
            <div className="space-y-2">
              {availability.map(({ method, possible, verdict }) => (
                <div
                  key={method.id}
                  className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-white/5 bg-white/[0.02] p-3.5"
                >
                  <div className="min-w-0">
                    <div className="flex flex-wrap items-center gap-2">
                      <span className="text-sm font-medium text-white">{method.name}</span>
                      <Pill tone={method.integration === "external" ? "accent" : method.integration === "native" ? "muted" : "bad"}>
                        {integrationLabel[method.integration]}
                      </Pill>
                    </div>
                    <p className="mt-1 text-xs text-muted">{verdict}</p>
                  </div>
                  <Pill tone={possible ? "good" : "bad"}>{possible ? "Possible" : "No"}</Pill>
                </div>
              ))}
            </div>
          </Card>

          <Card>
            <p className="mb-1 font-display text-base font-semibold text-white">
              Not detectable from a browser
            </p>
            <p className="mb-4 text-xs text-muted">
              These are deliberately blank rather than guessed. The desktop app reads them
              from WMI, the driver and the filesystem.
            </p>
            <div className="grid gap-x-8 sm:grid-cols-2">
              {BROWSER_BLIND_SPOTS.map((b) => (
                <KeyVal key={b.label} k={b.label} v={b.why} tone="muted" />
              ))}
            </div>
            <div className="mt-5">
              <a
                href={site.appExeUrl}
                className="inline-flex items-center rounded-lg bg-accent px-4 py-2 text-sm font-medium text-white shadow-glow transition-colors hover:bg-accentSoft"
              >
                Get full detection ({site.appVersion})
              </a>
            </div>
          </Card>
        </div>
      )}
    </div>
  );
}

// ---------------------------------------------------------------------------

function UpscalingPanel({
  draft,
  setDraft,
  system,
  availability,
  onSave,
}: {
  draft: Profile;
  setDraft: React.Dispatch<React.SetStateAction<Profile>>;
  system: DetectedSystem | null;
  availability: ReturnType<typeof resolveMethods>;
  onSave: () => void;
}) {
  const method = methodById(draft.method);
  const verdict = availability.find((a) => a.method.id === draft.method);
  const plan = planResolution(draft.outputWidth, draft.outputHeight, presetById(draft.quality).ratio);

  // Keep render resolution in sync with output + preset.
  useEffect(() => {
    setDraft((d) => {
      const p = planResolution(d.outputWidth, d.outputHeight, presetById(d.quality).ratio);
      if (p.renderWidth === d.renderWidth && p.renderHeight === d.renderHeight) return d;
      return { ...d, renderWidth: p.renderWidth, renderHeight: p.renderHeight };
    });
  }, [draft.outputWidth, draft.outputHeight, draft.quality, setDraft]);

  const selectable = METHODS.filter((m) =>
    draft.mode === "external" ? m.integration === "external" : m.integration !== "unavailable",
  );

  useEffect(() => {
    if (!selectable.some((m) => m.id === draft.method) && selectable[0]) {
      setDraft((d) => ({ ...d, method: selectable[0].id }));
    }
  }, [draft.mode, draft.method, selectable, setDraft]);

  return (
    <div>
      <PanelHeading
        kicker="Works without game support"
        title="Universal Upscaling"
        blurb="Spatial upscaling runs on the frames a window already presents, so it works with any game or application. Set the game's own resolution to the render resolution below; FrameFX scales its output up to your display."
      />

      <div className="mb-6 grid gap-4 lg:grid-cols-[1.1fr_1fr]">
        <Card>
          <p className="mb-4 font-display text-base font-semibold text-white">Target</p>
          <div className="grid gap-4 sm:grid-cols-2">
            <Field label="Profile name">
              <TextInput
                value={draft.name}
                placeholder="e.g. Cyberpunk 2077"
                onChange={(e) => setDraft((d) => ({ ...d, name: e.target.value }))}
              />
            </Field>
            <Field label="Executable / window">
              <TextInput
                value={draft.executablePath}
                placeholder="C:\\Games\\…\\game.exe"
                onChange={(e) => setDraft((d) => ({ ...d, executablePath: e.target.value }))}
              />
            </Field>
          </div>

          <div className="mt-4 grid gap-4 sm:grid-cols-2">
            <Field label="Mode" hint={draft.mode === "external" ? "Applied by FrameFX to any window." : "Configures the game's own implementation."}>
              <Select
                value={draft.mode}
                onChange={(e) => setDraft((d) => ({ ...d, mode: e.target.value as Profile["mode"] }))}
              >
                <option value="external">External — window-level</option>
                <option value="native">Native — the game&apos;s own</option>
              </Select>
            </Field>
            <Field label="Method">
              <Select
                value={draft.method}
                onChange={(e) => setDraft((d) => ({ ...d, method: e.target.value }))}
              >
                {selectable.map((m) => (
                  <option key={m.id} value={m.id}>
                    {m.name}
                  </option>
                ))}
              </Select>
            </Field>
          </div>

          {method ? (
            <div className="mt-4 rounded-xl border border-white/5 bg-white/[0.02] p-4">
              <div className="mb-2 flex flex-wrap items-center gap-2">
                <Pill tone={method.integration === "external" ? "accent" : method.integration === "native" ? "muted" : "bad"}>
                  {integrationLabel[method.integration]}
                </Pill>
                <Pill>{method.kind}</Pill>
                {method.requires ? <Pill tone="warn">{method.requires}</Pill> : null}
              </div>
              <p className="text-xs leading-relaxed text-muted">{method.note}</p>
              {verdict ? (
                <p className={`mt-2 text-xs ${verdict.possible ? "text-good" : "text-bad"}`}>
                  {verdict.verdict}
                </p>
              ) : null}
            </div>
          ) : null}

          <div className="mt-4 grid gap-4 sm:grid-cols-2">
            <Field label="Output resolution">
              <Select
                value={`${draft.outputWidth}x${draft.outputHeight}`}
                onChange={(e) => {
                  const [w, h] = e.target.value.split("x").map(Number);
                  setDraft((d) => ({ ...d, outputWidth: w, outputHeight: h }));
                }}
              >
                {COMMON_RESOLUTIONS.map((r) => (
                  <option key={r.label} value={`${r.w}x${r.h}`}>
                    {r.label}
                  </option>
                ))}
                {!COMMON_RESOLUTIONS.some((r) => r.w === draft.outputWidth && r.h === draft.outputHeight) ? (
                  <option value={`${draft.outputWidth}x${draft.outputHeight}`}>
                    {draft.outputWidth} × {draft.outputHeight} (detected)
                  </option>
                ) : null}
              </Select>
            </Field>
            <Field label="Quality preset">
              <Select
                value={draft.quality}
                onChange={(e) => setDraft((d) => ({ ...d, quality: e.target.value }))}
              >
                {QUALITY_PRESETS.map((q) => (
                  <option key={q.id} value={q.id}>
                    {q.name} ({q.ratio.toFixed(2)}×)
                  </option>
                ))}
              </Select>
            </Field>
          </div>

          <div className="mt-4">
            <Field label="Sharpening" hint="Applied after upscaling. 0 disables it.">
              <Slider value={draft.sharpness} onChange={(v) => setDraft((d) => ({ ...d, sharpness: v }))} />
            </Field>
          </div>

          <div className="mt-4 grid gap-4 sm:grid-cols-2">
            <Field label="Launch arguments" hint="Passed through when the app launches the game.">
              <TextInput
                value={draft.launchArgs}
                placeholder="-dx12 -fullscreen"
                onChange={(e) => setDraft((d) => ({ ...d, launchArgs: e.target.value }))}
              />
            </Field>
            <div className="flex flex-col justify-end gap-2 pb-1">
              <Toggle
                label="HDR passthrough"
                on={draft.hdr}
                onChange={(v) => setDraft((d) => ({ ...d, hdr: v }))}
              />
              <Toggle
                label="Low-latency mode"
                on={draft.lowLatency}
                onChange={(v) => setDraft((d) => ({ ...d, lowLatency: v }))}
              />
              <Toggle
                label="Safe mode (no game files written)"
                on={draft.safeMode}
                onChange={(v) => setDraft((d) => ({ ...d, safeMode: v }))}
              />
            </div>
          </div>
        </Card>

        <div className="space-y-4">
          <Card>
            <p className="mb-4 font-display text-base font-semibold text-white">Resolution plan</p>
            <div className="mb-4 rounded-xl border border-accent/25 bg-accent/5 p-4 text-center">
              <p className="font-mono text-[10px] uppercase tracking-widest text-faint">Set the game to</p>
              <p className="mt-1 font-display text-2xl font-bold text-gradient">
                {plan.renderWidth} × {plan.renderHeight}
              </p>
              <p className="mt-2 font-mono text-[10px] uppercase tracking-widest text-faint">
                FrameFX outputs
              </p>
              <p className="mt-1 font-display text-lg font-semibold text-white">
                {plan.outputWidth} × {plan.outputHeight}
              </p>
            </div>
            <KeyVal k="Scale factor" v={`${plan.ratio.toFixed(2)}× per axis`} />
            <KeyVal k="Pixels rendered" v={`${(plan.pixelFraction * 100).toFixed(1)}% of native`} />
            <KeyVal
              k="Expected effect"
              v={
                plan.ratio <= 1
                  ? "No upscaling — native rendering"
                  : "Lower GPU load; image reconstructed by the upscaler"
              }
            />
            <p className="mt-3 text-[11px] leading-relaxed text-faint">
              FrameFX cannot force a game to render at a lower resolution from outside. Set
              the resolution in the game, then this upscales its output.
            </p>
          </Card>

          <Card>
            <p className="mb-3 font-display text-base font-semibold text-white">Frame generation</p>
            <Field label="Native frame generation">
              <Select
                value={draft.frameGen}
                onChange={(e) =>
                  setDraft((d) => ({
                    ...d,
                    frameGen: e.target.value,
                    frameGenEnabled: e.target.value !== "None",
                  }))
                }
              >
                <option value="None">Off</option>
                <option value="FSR3-FG">FSR 3 Frame Generation (game must ship it)</option>
                <option value="XeSS-FG">XeSS Frame Generation (game must ship it)</option>
              </Select>
            </Field>
            <p className="mt-3 rounded-lg border border-bad/25 bg-bad/5 p-3 text-[11px] leading-relaxed text-muted">
              <span className="font-semibold text-bad">Cannot be injected.</span> Frame
              generation needs engine motion vectors and a proxied swapchain. FrameFX will
              only expose a game&apos;s own toggle — no tool can add FSR 3 or XeSS frame
              generation to a title that did not ship it.
            </p>
          </Card>

          <div className="flex flex-wrap gap-3">
            <Btn variant="primary" onClick={onSave}>
              Save as profile
            </Btn>
            <Btn onClick={() => downloadJson(`${draft.name || "profile"}.json`, draft)}>
              Export JSON
            </Btn>
          </div>
          {!system ? (
            <p className="text-xs text-faint">
              Tip: run a scan first and the output resolution defaults to your display.
            </p>
          ) : null}
        </div>
      </div>
    </div>
  );
}

function Toggle({
  label,
  on,
  onChange,
}: {
  label: string;
  on: boolean;
  onChange: (v: boolean) => void;
}) {
  return (
    <button
      onClick={() => onChange(!on)}
      className="flex items-center justify-between gap-3 rounded-lg border border-border bg-black/30 px-3 py-2 text-left text-xs text-white transition-colors hover:border-accent"
    >
      <span>{label}</span>
      <span
        className={`relative h-4 w-8 shrink-0 rounded-full transition-colors ${on ? "bg-accent" : "bg-white/15"}`}
      >
        <span
          className={`absolute top-0.5 h-3 w-3 rounded-full bg-white transition-all ${on ? "left-4" : "left-0.5"}`}
        />
      </span>
    </button>
  );
}

// ---------------------------------------------------------------------------

function SettingsPanel({
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
