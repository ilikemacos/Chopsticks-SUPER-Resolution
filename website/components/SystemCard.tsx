"use client";

import { useEffect, useState } from "react";
import Link from "next/link";
import { detectSystem, type DetectedSystem } from "@/lib/detect";
import { resolveMethods } from "@/lib/engine";

/**
 * Live system card. Shows a previous scan if one exists in this browser,
 * otherwise offers to run one. Every value shown is read from a real browser
 * API — the card stays empty rather than displaying placeholder hardware.
 */
export function SystemCard() {
  const [system, setSystem] = useState<DetectedSystem | null>(null);
  const [scanning, setScanning] = useState(false);
  const [ready, setReady] = useState(false);

  useEffect(() => {
    try {
      const raw = localStorage.getItem("framefx.system");
      if (raw) setSystem(JSON.parse(raw) as DetectedSystem);
    } catch {
      /* ignore */
    }
    setReady(true);
  }, []);

  const scan = async () => {
    setScanning(true);
    try {
      const result = await detectSystem();
      setSystem(result);
      try {
        localStorage.setItem("framefx.system", JSON.stringify(result));
      } catch {
        /* ignore */
      }
    } finally {
      setScanning(false);
    }
  };

  const usable = system
    // `possible === null` is undetermined, not yes. It is deliberately left out
    // of this list rather than counted as usable; the Scan panel shows it
    // explicitly with its reason.
    ? resolveMethods(system.gpu)
        .filter((a) => a.possible === true)
        .map((a) => a.method.name)
    : [];

  return (
    <div className="card-glass mx-auto mt-14 max-w-3xl rounded-2xl p-6 text-left">
      <div className="mb-4 flex flex-wrap items-center justify-between gap-3">
        <p className="section-label">Your system</p>
        {system ? (
          <span className="font-mono text-[10px] uppercase tracking-widest text-faint">
            Detected in browser
          </span>
        ) : null}
      </div>

      {!ready ? (
        <div className="h-20" />
      ) : !system ? (
        <div className="flex flex-wrap items-center justify-between gap-4">
          <p className="max-w-md text-sm text-muted">
            Detect your GPU and which upscaling technologies your hardware can actually
            run. Runs locally — nothing leaves your browser.
          </p>
          <button
            onClick={scan}
            disabled={scanning}
            className="shrink-0 rounded-xl bg-primary px-6 py-3 text-sm font-semibold text-primaryFg shadow-glow transition-colors hover:bg-primary/90 disabled:opacity-50"
          >
            {scanning ? "Scanning…" : "Scan my PC"}
          </button>
        </div>
      ) : (
        <div>
          <div className="grid gap-4 sm:grid-cols-3">
            <div className="sm:col-span-2">
              <p className="font-mono text-[10px] uppercase tracking-widest text-faint">GPU</p>
              <p className="mt-1 truncate font-display text-lg font-semibold text-fg">
                {system.glRenderer ?? "Not reported"}
              </p>
              <p className="mt-0.5 text-xs text-muted">
                {system.gpu ? `${system.gpu.vendor} · ${system.gpu.arch}` : "Vendor unknown"}
                {system.cpuThreads ? ` · ${system.cpuThreads} CPU threads` : ""}
              </p>
            </div>
            <div>
              <p className="font-mono text-[10px] uppercase tracking-widest text-faint">Display</p>
              <p className="mt-1 font-display text-lg font-semibold text-fg">
                {system.display.width} × {system.display.height}
              </p>
              <p className="mt-0.5 text-xs text-muted">
                {system.display.refreshHz ? `${system.display.refreshHz} Hz` : "refresh n/a"}
                {system.display.hdr ? " · HDR" : ""}
              </p>
            </div>
          </div>

          <div className="mt-5 flex flex-wrap gap-2">
            {usable.map((name) => (
              <span
                key={name}
                className="rounded-full border border-good/30 bg-good/10 px-3 py-1 font-mono text-[10px] uppercase tracking-wider text-good"
              >
                {name}
              </span>
            ))}
          </div>

          <div className="mt-5 flex flex-wrap items-center gap-3">
            <Link
              href="/console#scan"
              className="rounded-lg bg-primary px-4 py-2 text-sm font-semibold text-primaryFg transition-colors hover:bg-primary/90"
            >
              Full report
            </Link>
            <button
              onClick={scan}
              disabled={scanning}
              className="rounded-lg border border-border px-4 py-2 text-sm text-fg transition-colors hover:border-primary/30 disabled:opacity-50"
            >
              {scanning ? "Scanning…" : "Re-scan"}
            </button>
            <span className="text-[11px] text-faint">
              VRAM, driver and per-game data need the desktop app.
            </span>
          </div>
        </div>
      )}
    </div>
  );
}
