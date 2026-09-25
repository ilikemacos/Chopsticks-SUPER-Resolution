"use client";

import { useEffect } from "react";
import {
  Btn,
  Card,
  Field,
  KeyVal,
  PanelHeading,
  Pill,
  Select,
  Slider,
  TextInput,
  downloadJson,
} from "./ui";
import { type DetectedSystem } from "@/lib/detect";
import {
  COMMON_RESOLUTIONS,
  METHODS,
  QUALITY_PRESETS,
  integrationLabel,
  methodById,
  planResolution,
  presetById,
  resolveMethods,
  type Profile,
} from "@/lib/engine";

export function UpscalingPanel({
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
        blurb="CSR runs on the frames a window already presents, so it works with any game or application. Set the game's own resolution to the render resolution below; CSR scales its output up to your display. Temporal upscalers cannot work this way — they need motion vectors and depth that only the renderer has."
      />

      <div className="mb-6 grid gap-4 lg:grid-cols-[1.1fr_1fr]">
        <Card>
          <p className="mb-4 font-display text-base font-semibold text-fg">Target</p>
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
            <div className="mt-4 rounded-xl border border-border bg-card/40 p-4">
              <div className="mb-2 flex flex-wrap items-center gap-2">
                <Pill tone={method.integration === "external" ? "accent" : method.integration === "native" ? "muted" : "bad"}>
                  {integrationLabel[method.integration]}
                </Pill>
                <Pill>{method.kind}</Pill>
                {method.requires ? <Pill tone="warn">{method.requires}</Pill> : null}
              </div>
              <p className="text-xs leading-relaxed text-muted">{method.note}</p>
              {verdict ? (
                <p
                  className={`mt-2 text-xs ${
                    verdict.possible === true
                      ? "text-good"
                      : verdict.possible === null
                        ? "text-warn"
                        : "text-bad"
                  }`}
                >
                  {verdict.possible === null ? "Undetermined — " : ""}
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
            <p className="mb-4 font-display text-base font-semibold text-fg">Resolution plan</p>
            <div className="mb-4 rounded-xl border border-primary/25 bg-primary/5 p-4 text-center">
              <p className="font-mono text-[10px] uppercase tracking-widest text-faint">Set the game to</p>
              <p className="mt-1 font-display text-2xl font-bold text-gradient">
                {plan.renderWidth} × {plan.renderHeight}
              </p>
              <p className="mt-2 font-mono text-[10px] uppercase tracking-widest text-faint">
                FrameFX outputs
              </p>
              <p className="mt-1 font-display text-lg font-semibold text-fg">
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
            <p className="mb-3 font-display text-base font-semibold text-fg">Frame generation</p>
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
      className="flex items-center justify-between gap-3 rounded-lg border border-border bg-card px-3 py-2 text-left text-xs text-fg transition-colors hover:border-primary/30"
    >
      <span>{label}</span>
      <span
        className={`relative h-4 w-8 shrink-0 rounded-full transition-colors ${on ? "bg-primary" : "bg-mutedBg"}`}
      >
        <span
          className={`absolute top-0.5 h-3 w-3 rounded-full bg-primaryFg transition-all ${on ? "left-4" : "left-0.5"}`}
        />
      </span>
    </button>
  );
}
