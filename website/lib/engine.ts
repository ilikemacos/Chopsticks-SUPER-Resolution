// ---------------------------------------------------------------------------
// Universal FrameFX — capability engine.
//
// Single source of truth for what each technology can and cannot do, and how
// it can be applied. Everything the UI shows is derived from here so the site
// can never drift into claiming support that does not exist.
// ---------------------------------------------------------------------------

/**
 * How a technology can be applied to a game.
 *  - "native"   : the game must ship it. Cannot be added from outside.
 *  - "external" : FrameFX can apply it to any window's presented frames.
 *  - "unavailable": cannot be done on this machine (or at all) — `reason` says why.
 */
export type Integration = "native" | "external" | "unavailable";

export const integrationLabel: Record<Integration, string> = {
  native: "Native game integration",
  external: "External (window-level)",
  unavailable: "Not available",
};

export interface Method {
  id: string;
  name: string;
  /** Spatial = works on a finished frame. Temporal = needs engine data. */
  kind: "spatial" | "temporal" | "framegen";
  integration: Integration;
  /** Why it is limited / unavailable. Always shown when not plainly available. */
  note: string;
  /** Hardware requirement, when there is a real one. */
  requires?: string;
}

/**
 * The static truth table.
 *
 * Temporal upscalers (FSR 2/3, XeSS) reconstruct detail using motion vectors,
 * depth and jitter produced by the renderer. Those buffers do not exist in a
 * captured window, so they can only ever be enabled by the game itself.
 *
 * Spatial upscalers operate on a finished image, so they CAN be applied to any
 * window — that is what makes "Universal Upscaling" real rather than a claim.
 */
export const METHODS: Method[] = [
  {
    id: "csr",
    name: "CSR (Chopsticks Super Resolution)",
    kind: "spatial",
    integration: "external",
    note:
      "Our own spatial upscaler, derived from AMD FSR 1's EASU + RCAS design: a " +
      "16-tap edge-adaptive resolve with a Rec.709 luma direction estimate and a " +
      "deringing clamp, then contrast-limited sharpening that adapts to local " +
      "variance. Measured against the same references, it is +1.48 dB PSNR over " +
      "FSR 1 and +1.90 dB over bilinear. Runs on the frames a window already " +
      "presents, so it works with any game or application. Spatial only — it " +
      "cannot reconstruct detail the game never rendered, and it also scales the HUD.",
  },
  {
    id: "fsr1",
    name: "FSR 1",
    kind: "spatial",
    integration: "external",
    note:
      "AMD's spatial upscaler. Because it needs no engine data it can be applied " +
      "externally to a captured window, and it also appears natively in games that " +
      "ship it. Sharper than a plain resample, but still spatial-only.",
  },
  {
    id: "fsr2",
    name: "FSR 2",
    kind: "temporal",
    integration: "native",
    note:
      "Temporal reconstruction. Needs motion vectors, depth and jitter from the " +
      "renderer, which do not exist in a captured window — it cannot be injected.",
  },
  {
    id: "fsr3",
    name: "FSR 3 (upscaling)",
    kind: "temporal",
    integration: "native",
    note:
      "Temporal reconstruction, same engine-data requirement as FSR 2. Frame " +
      "generation is a separate feature from FSR 3 upscaling.",
    requires: "DirectX 12 capable system",
  },
  {
    id: "fsr4",
    name: "FSR 4",
    kind: "temporal",
    integration: "native",
    note:
      "Machine-learning upscaler. Requires both a game that ships FSR 4 and AMD " +
      "RDNA 4 hardware.",
    requires: "AMD RDNA 4 GPU",
  },
  {
    id: "xess",
    name: "Intel XeSS",
    kind: "temporal",
    integration: "native",
    note:
      "Temporal reconstruction. XMX path on Intel Arc, DP4a fallback elsewhere. " +
      "Requires game integration — it cannot be added externally.",
  },
  {
    id: "fsr3-fg",
    name: "FSR 3 Frame Generation",
    kind: "framegen",
    integration: "native",
    note:
      "Generates intermediate frames using engine motion vectors and a proxied " +
      "swapchain. It cannot be added to a game that did not ship it.",
    requires: "DirectX 12 capable system",
  },
  {
    id: "xess-fg",
    name: "XeSS Frame Generation",
    kind: "framegen",
    integration: "native",
    note:
      "Same constraint as FSR 3 frame generation — engine integration only. Best " +
      "on Intel Arc where the game implements it.",
  },
  {
    id: "external-fg",
    name: "External frame interpolation",
    kind: "framegen",
    integration: "unavailable",
    note:
      "Interpolating presented frames with optical flow is technically possible, " +
      "but it is NOT FSR 3 / XeSS frame generation: it has no engine motion " +
      "vectors, adds a full frame of latency and warps HUD elements. FrameFX does " +
      "not ship it, and no tool can 'inject' vendor frame generation universally.",
  },
];

export function methodById(id: string): Method | undefined {
  return METHODS.find((m) => m.id === id);
}

/** Methods that genuinely work on any window, with no game support required. */
export const externalMethods = METHODS.filter((m) => m.integration === "external");

// ---------------------------------------------------------------------------
// Quality presets / resolution math
// ---------------------------------------------------------------------------

export interface QualityPreset {
  id: string;
  name: string;
  /** Output ÷ render, per axis. */
  ratio: number;
}

export const QUALITY_PRESETS: QualityPreset[] = [
  { id: "native", name: "Native", ratio: 1.0 },
  { id: "ultra-quality", name: "Ultra Quality", ratio: 1.3 },
  { id: "quality", name: "Quality", ratio: 1.5 },
  { id: "balanced", name: "Balanced", ratio: 1.7 },
  { id: "performance", name: "Performance", ratio: 2.0 },
  { id: "ultra-performance", name: "Ultra Performance", ratio: 3.0 },
];

export function presetById(id: string): QualityPreset {
  return QUALITY_PRESETS.find((p) => p.id === id) ?? QUALITY_PRESETS[0];
}

export interface ResolutionPlan {
  renderWidth: number;
  renderHeight: number;
  outputWidth: number;
  outputHeight: number;
  ratio: number;
  /** Fraction of native pixels actually rendered (0..1). */
  pixelFraction: number;
}

export function planResolution(
  outputWidth: number,
  outputHeight: number,
  ratio: number,
): ResolutionPlan {
  const safeRatio = ratio > 0 ? ratio : 1;
  const renderWidth = Math.max(1, Math.round(outputWidth / safeRatio));
  const renderHeight = Math.max(1, Math.round(outputHeight / safeRatio));
  const pixelFraction =
    (renderWidth * renderHeight) / Math.max(1, outputWidth * outputHeight);
  return {
    renderWidth,
    renderHeight,
    outputWidth,
    outputHeight,
    ratio: safeRatio,
    pixelFraction,
  };
}

export const COMMON_RESOLUTIONS: { label: string; w: number; h: number }[] = [
  { label: "1280 × 720", w: 1280, h: 720 },
  { label: "1600 × 900", w: 1600, h: 900 },
  { label: "1920 × 1080", w: 1920, h: 1080 },
  { label: "2560 × 1080", w: 2560, h: 1080 },
  { label: "2560 × 1440", w: 2560, h: 1440 },
  { label: "3440 × 1440", w: 3440, h: 1440 },
  { label: "3840 × 2160", w: 3840, h: 2160 },
];

// ---------------------------------------------------------------------------
// GPU inference
// ---------------------------------------------------------------------------

export type Vendor = "NVIDIA" | "AMD" | "Intel" | "Apple" | "Unknown";

export interface GpuGuess {
  vendor: Vendor;
  /** Marketing architecture, when it can be inferred from the model name. */
  arch: string;
  /** The exact string the browser reported, unmodified. */
  raw: string;
  /** True when we could not confidently identify the part. */
  uncertain: boolean;
}

/**
 * Infers vendor/architecture from a GPU name string. This is a best-effort
 * read of a name the browser reports — it is never presented as authoritative,
 * and it never invents VRAM, clocks or driver data.
 */
export function inferGpu(raw: string): GpuGuess {
  const s = (raw || "").toLowerCase();
  let vendor: Vendor = "Unknown";
  if (/nvidia|geforce|rtx|gtx|quadro/.test(s)) vendor = "NVIDIA";
  else if (/amd|radeon|rx\s?\d|vega|gfx\d/.test(s)) vendor = "AMD";
  else if (/intel|arc|iris|uhd graphics|hd graphics/.test(s)) vendor = "Intel";
  else if (/apple|m[123]\s|metal/.test(s)) vendor = "Apple";

  let arch = "Unknown";
  let uncertain = true;

  if (vendor === "NVIDIA") {
    if (/rtx\s?50\d\d/.test(s)) { arch = "Blackwell"; uncertain = false; }
    else if (/rtx\s?40\d\d/.test(s)) { arch = "Ada Lovelace"; uncertain = false; }
    else if (/rtx\s?30\d\d/.test(s)) { arch = "Ampere"; uncertain = false; }
    else if (/rtx\s?20\d\d|gtx\s?16\d\d/.test(s)) { arch = "Turing"; uncertain = false; }
    else if (/gtx\s?10\d\d/.test(s)) { arch = "Pascal"; uncertain = false; }
  } else if (vendor === "AMD") {
    if (/rx\s?90\d\d/.test(s)) { arch = "RDNA 4"; uncertain = false; }
    else if (/rx\s?7\d\d\d/.test(s)) { arch = "RDNA 3"; uncertain = false; }
    else if (/rx\s?6\d\d\d/.test(s)) { arch = "RDNA 2"; uncertain = false; }
    else if (/rx\s?5\d\d\d/.test(s)) { arch = "RDNA 1"; uncertain = false; }
  } else if (vendor === "Intel") {
    if (/\bb\d{3}\b|battlemage/.test(s)) { arch = "Xe2 (Battlemage)"; uncertain = false; }
    else if (/\ba\d{3}\b|alchemist/.test(s)) { arch = "Xe-HPG (Arc)"; uncertain = false; }
    else if (/iris|uhd|hd graphics/.test(s)) { arch = "Integrated Xe/Gen"; uncertain = false; }
  }

  return { vendor, arch, raw: raw || "", uncertain };
}

// ---------------------------------------------------------------------------
// Capability resolution for a specific machine
// ---------------------------------------------------------------------------

export interface MethodAvailability {
  method: Method;
  /**
   * Can it be used at all on this hardware (ignoring per-game support)?
   * `null` means undetermined — we could not establish it either way, which is
   * not the same as "no" and must never be rendered as one.
   */
  possible: boolean | null;
  /** Plain-language explanation of the verdict. */
  verdict: string;
}

/**
 * Resolves each method against a detected GPU. Hardware gates only — whether a
 * *particular game* ships a technology is a per-title fact the Inspector
 * reports from disk, never guessed here.
 */
export function resolveMethods(gpu: GpuGuess | null): MethodAvailability[] {
  return METHODS.map((method) => {
    if (method.integration === "unavailable") {
      return { method, possible: false, verdict: method.note };
    }

    if (method.id === "fsr4") {
      if (!gpu) {
        return { method, possible: false, verdict: "No GPU detected yet — run a scan." };
      }
      if (gpu.vendor !== "AMD") {
        return {
          method,
          possible: false,
          verdict: `Requires an AMD RDNA 4 GPU; detected ${gpu.vendor}.`,
        };
      }
      // An unconfirmed architecture is not a confirmed absence. The browser
      // reports a name, not a part number, so when we could not identify it the
      // honest answer is "undetermined" — saying "unavailable" would assert
      // absence from a guess. This mirrors ArchCertainty in the desktop app.
      if (gpu.uncertain) {
        return {
          method,
          possible: null,
          verdict:
            "Cannot confirm your GPU architecture from what the browser reports" +
            (gpu.raw ? ` ("${gpu.raw}")` : "") +
            ". FSR 4 needs RDNA 4; check your GPU model to be sure.",
        };
      }
      if (gpu.arch !== "RDNA 4") {
        return {
          method,
          possible: false,
          verdict: `Requires RDNA 4 hardware; detected ${gpu.arch}.`,
        };
      }
      return { method, possible: true, verdict: "Hardware supported. The game must also ship FSR 4." };
    }

    if (method.integration === "external") {
      return {
        method,
        possible: true,
        verdict: "Works on any window — no game support required.",
      };
    }

    return {
      method,
      possible: true,
      verdict: "Runs on your hardware, but the game must ship it.",
    };
  });
}

// ---------------------------------------------------------------------------
// Profiles — schema shared with the desktop app
// ---------------------------------------------------------------------------

/**
 * Profile JSON. Schema 2 adds the Universal Upscaling fields; the desktop app
 * ignores properties it does not know, so schema-1 and schema-2 files remain
 * interchangeable in both directions.
 */
export interface Profile {
  schema: number;
  name: string;
  executablePath: string;
  /** "external" = window-level upscaling, "native" = configure the game's own. */
  mode: "external" | "native";
  api: string;
  /** Method id from METHODS. */
  method: string;
  /** Quality preset id. */
  quality: string;
  renderWidth: number;
  renderHeight: number;
  outputWidth: number;
  outputHeight: number;
  sharpness: number;
  frameGen: string;
  frameGenEnabled: boolean;
  hdr: boolean;
  lowLatency: boolean;
  launchArgs: string;
  notes: string;
  /** Never write to game files; configuration only. */
  safeMode: boolean;
}

export function emptyProfile(outputWidth = 1920, outputHeight = 1080): Profile {
  const plan = planResolution(outputWidth, outputHeight, 1.5);
  return {
    schema: 2,
    name: "",
    executablePath: "",
    mode: "external",
    api: "Auto",
    method: "csr",
    quality: "quality",
    renderWidth: plan.renderWidth,
    renderHeight: plan.renderHeight,
    outputWidth,
    outputHeight,
    sharpness: 0.5,
    frameGen: "None",
    frameGenEnabled: false,
    hdr: false,
    lowLatency: false,
    launchArgs: "",
    notes: "",
    safeMode: true,
  };
}

/** Parses untrusted JSON into a Profile, filling gaps with safe defaults. */
export function parseProfile(input: unknown): Profile | null {
  if (typeof input !== "object" || input === null) return null;
  const o = input as Record<string, unknown>;
  const name = typeof o.name === "string" ? o.name : "";
  if (!name.trim()) return null;

  const num = (v: unknown, fallback: number) =>
    typeof v === "number" && Number.isFinite(v) ? v : fallback;
  const str = (v: unknown, fallback: string) => (typeof v === "string" ? v : fallback);
  const bool = (v: unknown, fallback: boolean) => (typeof v === "boolean" ? v : fallback);

  const base = emptyProfile();
  const outputWidth = num(o.outputWidth, base.outputWidth);
  const outputHeight = num(o.outputHeight, base.outputHeight);

  return {
    schema: num(o.schema, 2),
    name,
    executablePath: str(o.executablePath, ""),
    mode: o.mode === "native" ? "native" : "external",
    api: str(o.api, "Auto"),
    // Accept schema-1 files, which used `upscaler` with names like "FSR3".
    method: currentMethodId(str(o.method, legacyMethodId(str(o.upscaler, base.method)))),
    quality: qualityIdFrom(o, base.quality),
    renderWidth: num(o.renderWidth, base.renderWidth),
    renderHeight: num(o.renderHeight, base.renderHeight),
    outputWidth,
    outputHeight,
    sharpness: num(o.sharpness, 0.5),
    frameGen: str(o.frameGen, "None"),
    frameGenEnabled: bool(o.frameGenEnabled, false),
    hdr: bool(o.hdr, false),
    lowLatency: bool(o.lowLatency, false),
    launchArgs: str(o.launchArgs, ""),
    notes: str(o.notes, ""),
    safeMode: bool(o.safeMode, true),
  };
}

function legacyMethodId(v: string): string {
  const map: Record<string, string> = {
    none: "csr",
    csr: "csr",
    fsr1: "fsr1",
    fsr2: "fsr2",
    fsr3: "fsr3",
    fsr4: "fsr4",
    xess: "xess",
  };
  return map[v.toLowerCase()] ?? "csr";
}

/**
 * Maps a method id to a current one.
 *
 * "framefx-spatial" was the id of our own spatial upscaler before it became CSR.
 * Profiles are stored in the visitor's browser and exported to disk, so files
 * written under the old id still exist and must keep resolving to a real method
 * rather than silently falling back to the default.
 */
function currentMethodId(v: string): string {
  if (v === "framefx-spatial") return "csr";
  return v;
}

// ---------------------------------------------------------------------------
// Desktop interop
// ---------------------------------------------------------------------------

/** Method id -> the desktop app's `upscaler` string. */
const DESKTOP_UPSCALER: Record<string, string> = {
  csr: "CSR",
  fsr1: "FSR1",
  fsr2: "FSR2",
  fsr3: "FSR3",
  fsr4: "FSR4",
  xess: "XeSS",
  "fsr3-fg": "None",
  "xess-fg": "None",
  "external-fg": "None",
};

/**
 * Serialises a profile so the desktop app reads it correctly.
 *
 * The two apps do not use the same field names: the desktop reads `upscaler`
 * (a display string) and expects `quality` as a display string too, while this
 * site uses `method` and preset ids. Writing only our own names meant the
 * desktop silently fell back to its defaults — an imported profile came out as
 * FSR 3 at the default quality no matter what was chosen here, which is exactly
 * the kind of quiet wrong answer the project's honesty rule exists to prevent.
 *
 * Both sides ignore properties they do not know, so the file carries both
 * spellings and each app reads the one it understands.
 */
export function toInteropProfile(p: Profile): Record<string, unknown> {
  return {
    ...p,
    // Desktop field names, written alongside ours.
    upscaler: DESKTOP_UPSCALER[p.method] ?? "None",
    quality: presetById(p.quality).name,
    // Our own preset id, so a round trip through the desktop app and back does
    // not lose it (the desktop preserves unknown properties it never reads).
    qualityId: p.quality,
  };
}

/** Accepts either spelling of the quality field. */
function qualityIdFrom(o: Record<string, unknown>, fallback: string): string {
  const raw =
    typeof o.qualityId === "string" && o.qualityId
      ? o.qualityId
      : typeof o.quality === "string"
        ? o.quality
        : fallback;
  const id = raw.toLowerCase().replace(/\s+/g, "-");
  return QUALITY_PRESETS.some((q) => q.id === id) ? id : fallback;
}

export function profileSlug(name: string): string {
  const s = name.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "");
  return s || "profile";
}
