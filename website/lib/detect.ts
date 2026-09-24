// ---------------------------------------------------------------------------
// Browser-side system detection.
//
// Everything here is read from a real browser API. Where a fact simply cannot
// be obtained in a browser (VRAM, driver version, temperatures, installed
// games, D3D feature levels) it is reported as unavailable rather than
// estimated — the desktop app is what reads those from the system.
// ---------------------------------------------------------------------------

import { inferGpu, type GpuGuess } from "./engine";

export interface DetectedDisplay {
  width: number;
  height: number;
  dpr: number;
  /** Measured with requestAnimationFrame, so it is a real observation. */
  refreshHz: number | null;
  hdr: boolean | null;
  colorGamut: string;
}

export interface DetectedSystem {
  gpu: GpuGuess | null;
  /** Raw strings exactly as the browser reported them. */
  glRenderer: string | null;
  glVendor: string | null;
  webgl2: boolean;
  webgpu: boolean;
  webgpuInfo: string | null;
  cpuThreads: number | null;
  /** Chrome-only, coarse RAM bucket in GB. */
  deviceMemoryGb: number | null;
  platform: string;
  display: DetectedDisplay;
  detectedAt: string;
}

/** Facts a browser cannot obtain, listed so the UI never has to invent them. */
export const BROWSER_BLIND_SPOTS: { label: string; why: string }[] = [
  { label: "VRAM size", why: "No browser API exposes video memory. Read from the driver by the desktop app." },
  { label: "Driver version", why: "Not exposed to web pages. The desktop app reads it from the system." },
  { label: "GPU / CPU utilisation", why: "Requires OS performance counters." },
  { label: "Temperatures", why: "Requires vendor/OS sensor access." },
  { label: "DirectX / Vulkan support", why: "A browser can only see WebGL/WebGPU, not D3D feature levels." },
  { label: "Installed games", why: "Web pages cannot enumerate the filesystem or registry." },
  { label: "In-game FPS", why: "A page can only measure its own rendering, never another process." },
];

function readWebglStrings(): { renderer: string | null; vendor: string | null; webgl2: boolean } {
  if (typeof document === "undefined") return { renderer: null, vendor: null, webgl2: false };
  try {
    const canvas = document.createElement("canvas");
    const gl =
      (canvas.getContext("webgl2") as WebGL2RenderingContext | null) ??
      (canvas.getContext("webgl") as WebGLRenderingContext | null);
    if (!gl) return { renderer: null, vendor: null, webgl2: false };

    const webgl2 = typeof WebGL2RenderingContext !== "undefined" && gl instanceof WebGL2RenderingContext;
    const dbg = gl.getExtension("WEBGL_debug_renderer_info");
    const renderer = dbg
      ? (gl.getParameter(dbg.UNMASKED_RENDERER_WEBGL) as string)
      : (gl.getParameter(gl.RENDERER) as string);
    const vendor = dbg
      ? (gl.getParameter(dbg.UNMASKED_VENDOR_WEBGL) as string)
      : (gl.getParameter(gl.VENDOR) as string);
    return { renderer: renderer ?? null, vendor: vendor ?? null, webgl2 };
  } catch {
    return { renderer: null, vendor: null, webgl2: false };
  }
}

async function readWebgpu(): Promise<{ supported: boolean; info: string | null }> {
  try {
    const nav = navigator as Navigator & { gpu?: { requestAdapter(): Promise<unknown> } };
    if (!nav.gpu) return { supported: false, info: null };
    const adapter = (await nav.gpu.requestAdapter()) as
      | { info?: { vendor?: string; architecture?: string; device?: string; description?: string } }
      | null;
    if (!adapter) return { supported: false, info: null };
    const i = adapter.info;
    if (!i) return { supported: true, info: null };
    const parts = [i.vendor, i.architecture, i.device, i.description].filter(
      (p): p is string => typeof p === "string" && p.length > 0,
    );
    return { supported: true, info: parts.length ? parts.join(" · ") : null };
  } catch {
    return { supported: false, info: null };
  }
}

/** Measures refresh rate over a short burst of animation frames. */
function measureRefreshHz(sampleFrames = 40): Promise<number | null> {
  return new Promise((resolve) => {
    if (typeof requestAnimationFrame === "undefined") return resolve(null);
    const times: number[] = [];
    let frames = 0;
    const tick = (t: number) => {
      times.push(t);
      frames += 1;
      if (frames < sampleFrames) requestAnimationFrame(tick);
      else {
        const deltas: number[] = [];
        for (let i = 1; i < times.length; i++) deltas.push(times[i] - times[i - 1]);
        deltas.sort((a, b) => a - b);
        const median = deltas[Math.floor(deltas.length / 2)];
        resolve(median > 0 ? Math.round(1000 / median) : null);
      }
    };
    requestAnimationFrame(tick);
    // Never hang the UI if frames stop arriving (e.g. background tab).
    setTimeout(() => resolve(null), 2000);
  });
}

function readDisplayStatics(): Omit<DetectedDisplay, "refreshHz"> {
  const mm = (q: string) =>
    typeof window !== "undefined" && typeof window.matchMedia === "function"
      ? window.matchMedia(q).matches
      : false;

  let hdr: boolean | null = null;
  try {
    // `dynamic-range` is a real media feature; absence means "cannot tell".
    if (typeof window !== "undefined" && window.matchMedia("(dynamic-range: standard)").matches) {
      hdr = mm("(dynamic-range: high)");
    }
  } catch {
    hdr = null;
  }

  const gamut = mm("(color-gamut: rec2020)")
    ? "Rec. 2020"
    : mm("(color-gamut: p3)")
      ? "Display-P3"
      : mm("(color-gamut: srgb)")
        ? "sRGB"
        : "Unknown";

  return {
    width: typeof screen !== "undefined" ? screen.width : 0,
    height: typeof screen !== "undefined" ? screen.height : 0,
    dpr: typeof window !== "undefined" ? window.devicePixelRatio || 1 : 1,
    hdr,
    colorGamut: gamut,
  };
}

function readPlatform(): string {
  const nav = navigator as Navigator & {
    userAgentData?: { platform?: string };
  };
  if (nav.userAgentData?.platform) return nav.userAgentData.platform;
  if (/windows/i.test(navigator.userAgent)) return "Windows";
  if (/mac os x/i.test(navigator.userAgent)) return "macOS";
  if (/linux/i.test(navigator.userAgent)) return "Linux";
  return "Unknown";
}

/** Runs every available browser probe. Safe to call only in the browser. */
export async function detectSystem(): Promise<DetectedSystem> {
  const { renderer, vendor, webgl2 } = readWebglStrings();
  const [gpuRes, refreshHz] = await Promise.all([readWebgpu(), measureRefreshHz()]);

  const nav = navigator as Navigator & { deviceMemory?: number };

  return {
    gpu: renderer ? inferGpu(renderer) : null,
    glRenderer: renderer,
    glVendor: vendor,
    webgl2,
    webgpu: gpuRes.supported,
    webgpuInfo: gpuRes.info,
    cpuThreads: typeof navigator.hardwareConcurrency === "number" ? navigator.hardwareConcurrency : null,
    deviceMemoryGb: typeof nav.deviceMemory === "number" ? nav.deviceMemory : null,
    platform: readPlatform(),
    display: { ...readDisplayStatics(), refreshHz },
    detectedAt: new Date().toISOString(),
  };
}
