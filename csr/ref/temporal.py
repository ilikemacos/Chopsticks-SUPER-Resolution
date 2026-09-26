"""Temporal reconstruction reference -- the FSR 2 / DLSS *skeleton* without the
neural net, for measurement only (see csr/TEMPORAL.md).

Mechanism: run the spatial CSR upscaler on each low-res frame, then accumulate
those per-frame estimates across time. Because each frame is jittered by a
different sub-pixel offset, aligning and averaging many frames reduces aliasing
and recovers detail a single frame cannot hold -- this is why temporal upscalers
beat spatial ones. Alignment needs a motion field:

  mode="true"       -- the exact backward reprojection authored in scenes_motion
                       (a perfect engine motion vector). The honest ceiling.
  mode="estimated"  -- a global translation estimated from the finished frames
                       alone (what a capture-only external tool could obtain).

A neighborhood min/max clamp rectifies reprojected history against the current
frame, which is what rejects stale/occluded history (disocclusion).

This is deliberately NOT a neural network: DLSS's learned reconstruction is the
extra piece this cannot reach, and that is the point being measured.
"""
from __future__ import annotations

import numpy as np

from csr import csr_upscale, CsrConfig


def _warp(img: np.ndarray, coords: np.ndarray) -> np.ndarray:
    """Bilinear-sample img at absolute source coords (H,W,2)=(x,y), edge-clamped."""
    h, w = img.shape[:2]
    x = np.clip(coords[..., 0], 0, w - 1)
    y = np.clip(coords[..., 1], 0, h - 1)
    x0 = np.floor(x).astype(np.int64); x1 = np.minimum(x0 + 1, w - 1)
    y0 = np.floor(y).astype(np.int64); y1 = np.minimum(y0 + 1, h - 1)
    fx = (x - x0)[..., None]; fy = (y - y0)[..., None]
    a = img[y0, x0]; b = img[y0, x1]; c = img[y1, x0]; d = img[y1, x1]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def _neighbour_bounds(img: np.ndarray):
    """Per-pixel 3x3 min/max per channel (edge-replicated)."""
    p = np.pad(img, ((1, 1), (1, 1), (0, 0)), mode="edge")
    lo = np.full_like(img, 1.0)
    hi = np.full_like(img, 0.0)
    for dy in range(3):
        for dx in range(3):
            s = p[dy:dy + img.shape[0], dx:dx + img.shape[1]]
            lo = np.minimum(lo, s)
            hi = np.maximum(hi, s)
    return lo, hi


def estimate_translation(prev: np.ndarray, cur: np.ndarray):
    """Global Lucas-Kanade translation from two finished frames (luma).

    Returns (u, v): the apparent motion of content from prev to cur, so history
    is fetched at (x - u, y - v). Cheap and robust for panning; for rotation /
    zoom a single global vector is wrong away from the centre, and that error is
    exactly the external penalty this study measures."""
    a = prev.mean(axis=-1).astype(np.float64)
    b = cur.mean(axis=-1).astype(np.float64)
    gy, gx = np.gradient(0.5 * (a + b))
    it = b - a
    Ixx = float(np.sum(gx * gx)); Iyy = float(np.sum(gy * gy))
    Ixy = float(np.sum(gx * gy))
    Ixt = float(np.sum(gx * it)); Iyt = float(np.sum(gy * it))
    det = Ixx * Iyy - Ixy * Ixy
    if abs(det) < 1e-6:
        return 0.0, 0.0
    # [u, v] = A^-1 (-[Ixt, Iyt]); A^-1 = 1/det [[Iyy,-Ixy],[-Ixy,Ixx]].
    u = (-Iyy * Ixt + Ixy * Iyt) / det
    v = (Ixy * Ixt - Ixx * Iyt) / det
    return u, v


def reconstruct(frames, mode: str = "true", alpha: float = 0.85,
                cfg: CsrConfig = CsrConfig()):
    """Return a list of HR outputs, one per input frame."""
    wh, hh = frames[0]["wh"], frames[0]["hh"]
    gx, gy = np.meshgrid(np.arange(wh, dtype=np.float64),
                         np.arange(hh, dtype=np.float64))
    outs = []
    history = None
    prev_s = None
    for i, fr in enumerate(frames):
        s = csr_upscale(fr["lr"].astype(np.float64), wh, hh, cfg)   # spatial CSR
        if history is None:
            out = s
        else:
            if mode == "true":
                back = fr["back"].astype(np.float64)
            else:
                u, v = estimate_translation(prev_s, s)
                back = np.stack([gx - u, gy - v], axis=-1)
            reproj = _warp(history, back)
            lo, hi = _neighbour_bounds(s)
            reproj = np.clip(reproj, lo, hi)              # reject stale history
            out = (1.0 - alpha) * s + alpha * reproj
        out = np.clip(out, 0.0, 1.0)
        outs.append(out)
        history = out
        prev_s = s
    return outs
