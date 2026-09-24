"use client";

import { useCallback, useEffect, useMemo, useState } from "react";
import { downloadJson } from "./ui";
import { ScanPanel } from "./ScanPanel";
import { UpscalingPanel } from "./UpscalingPanel";
import { SettingsPanel } from "./SettingsPanel";
import {
  BenchmarksPanel,
  InspectorPanel,
  LibraryPanel,
  ProfilesPanel,
  type BenchmarkSet,
  type InspectionReport,
  type LibraryEntry,
} from "./DataPanels";
import { detectSystem, type DetectedSystem } from "@/lib/detect";
import {
  emptyProfile,
  parseProfile,
  planResolution,
  presetById,
  resolveMethods,
  type Profile,
} from "@/lib/engine";

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
