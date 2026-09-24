"use client";

import { Btn, Card, Empty, KeyVal, PanelHeading, Pill } from "./ui";
import { BROWSER_BLIND_SPOTS, type DetectedSystem } from "@/lib/detect";
import { integrationLabel, resolveMethods } from "@/lib/engine";
import { site } from "@/lib/site";

export function ScanPanel({
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
                <p className="font-display text-base font-semibold text-fg">Graphics</p>
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
              <p className="mb-3 font-display text-base font-semibold text-fg">System &amp; display</p>
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
            <p className="mb-1 font-display text-base font-semibold text-fg">
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
                  className="flex flex-wrap items-center justify-between gap-3 rounded-xl border border-border bg-card/40 p-3.5"
                >
                  <div className="min-w-0">
                    <div className="flex flex-wrap items-center gap-2">
                      <span className="text-sm font-medium text-fg">{method.name}</span>
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
            <p className="mb-1 font-display text-base font-semibold text-fg">
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
                className="inline-flex items-center rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90"
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
