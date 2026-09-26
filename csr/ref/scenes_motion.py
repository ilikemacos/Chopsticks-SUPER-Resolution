"""Synthetic moving scenes with KNOWN ground-truth motion.

The whole point of the temporal study (csr/TEMPORAL.md) is to measure the gap
between a temporal upscaler given *perfect* engine motion vectors and one that
must *estimate* motion from finished frames. That measurement is only honest if
we know the true motion exactly -- so the scenes here are authored analytically:
a continuous world function sampled through a known per-frame camera pose.

For each frame we produce:
  lr      -- the low-res input frame (area-averaged, sub-pixel jittered, like a
             renderer's jittered output that a temporal upscaler consumes).
  gt      -- the high-res ground truth at that frame's *base* pose (no jitter):
             what a perfect upscaler would output.
  back    -- the exact backward-reprojection coordinates that map each HR output
             pixel at frame t to its location in frame t-1's HR grid. This is the
             "perfect motion vector" a game engine would hand DLSS/FSR 2.

Coordinates are in HR "screen" space [0,Wh) x [0,Hh); the LR grid samples the
same screen region at lower density, so lr and gt image the same world.
"""
from __future__ import annotations

import numpy as np

# --- continuous world textures (vectorised over coordinate arrays) -----------


def _tex_detail(wx: np.ndarray, wy: np.ndarray) -> np.ndarray:
    """A high-frequency, colour-varying texture: fine detail is exactly what a
    single low-res frame cannot hold and multi-frame accumulation can recover."""
    r = 0.5 + 0.25 * np.sin(wx * 0.55) + 0.2 * np.sin((wx + wy) * 0.31)
    g = 0.5 + 0.25 * np.sin(wy * 0.62 + 1.3) + 0.2 * np.sin((wx - wy) * 0.27)
    b = 0.5 + 0.22 * np.sin((wx * 0.8 + wy * 0.2) + 2.1)
    # A fine checker adds hard high-frequency content near the sampling limit.
    checker = (((np.floor(wx / 3.0) + np.floor(wy / 3.0)).astype(np.int64) & 1)
               .astype(np.float64))
    m = 0.85 + 0.15 * checker
    return np.clip(np.stack([r * m, g * m, b * m], axis=-1), 0.0, 1.0)


def _sample(fn, sx: np.ndarray, sy: np.ndarray, pose, ss: int) -> np.ndarray:
    """Area-average fn over each screen pixel's footprint (ss x ss supersamples),
    mapping screen coords through `pose` (center, scale, offset) to world."""
    center, z, off = pose
    acc = None
    for oy in range(ss):
        for ox in range(ss):
            jx = (ox + 0.5) / ss - 0.5
            jy = (oy + 0.5) / ss - 0.5
            wx = center[0] + (sx + jx - center[0]) * z + off[0]
            wy = center[1] + (sy + jy - center[1]) * z + off[1]
            c = fn(wx, wy)
            acc = c if acc is None else acc + c
    return acc / (ss * ss)


def _halton(i: int, base: int) -> float:
    f, r = 1.0, 0.0
    while i > 0:
        f /= base
        r += f * (i % base)
        i //= base
    return r


def make_scene(name: str, wl: int, hl: int, ratio: float = 2.0,
               frames: int = 20, ss: int = 3):
    """Return a list of per-frame dicts for `name`.

    name: 'pan' (pure translation -- best case for motion estimation),
          'zoom' (scale about centre -- a single global translation cannot model
           it, so estimated motion misaligns), or
          'disocclusion' (a fast occluder reveals fresh background -- history is
           simply invalid where new content appears).
    """
    wh, hh = int(round(wl * ratio)), int(round(hl * ratio))
    center = np.array([wh / 2.0, hh / 2.0])

    ix = np.arange(wh) + 0.5
    iy = np.arange(hh) + 0.5
    sxh, syh = np.meshgrid(ix, iy)                      # HR screen centres
    lxs = (np.arange(wl) + 0.5) * (wh / wl)
    lys = (np.arange(hl) + 0.5) * (hh / hl)
    sxl, syl = np.meshgrid(lxs, lys)                    # LR screen centres

    def pose_of(t: int):
        if name == "zoom":
            z = 1.0 - 0.012 * t                         # slow zoom-in
            off = np.array([0.20 * t, 0.10 * t])
        else:
            z = 1.0
            off = np.array([0.9 * t, 0.55 * t])         # diagonal pan
        return (center, z, off)

    # A moving occluder (disocclusion scene): a vertical bar sweeping right,
    # revealing background that was hidden the frame before -> no valid history.
    def occluder_x(t: int) -> float:
        return -0.15 * wh + (1.3 * wh) * (t / max(1, frames - 1))

    out = []
    for t in range(frames):
        center_t, z_t, off_t = pose_of(t)
        # Sub-pixel TAA jitter (Halton 2,3), up to ~1 HR pixel, LR sampling only.
        jx = (_halton(t + 1, 2) - 0.5) * (wh / wl)
        jy = (_halton(t + 1, 3) - 0.5) * (hh / hl)

        def scene_fn(wx, wy, t=t):
            base = _tex_detail(wx, wy)
            if name != "disocclusion":
                return base
            ox = occluder_x(t)
            inside = (wx >= ox) & (wx < ox + 0.18 * wh)
            fg = np.empty_like(base)
            fg[..., 0] = 0.05
            fg[..., 1] = 0.05
            fg[..., 2] = 0.08
            return np.where(inside[..., None], fg, base)

        lr = _sample(scene_fn, sxl + jx, syl + jy, (center_t, z_t, off_t), ss)
        gt = _sample(scene_fn, sxh, syh, (center_t, z_t, off_t), ss)

        # Exact backward reprojection: HR pixel p at frame t -> its screen pos at
        # frame t-1 for the SAME world point. Absolute HR source coords (index).
        if t == 0:
            back = None
        else:
            _, z_p, off_p = pose_of(t - 1)
            wx = center_t[0] + (sxh - center_t[0]) * z_t + off_t[0]
            wy = center_t[1] + (syh - center_t[1]) * z_t + off_t[1]
            spx = center_t[0] + (wx - off_p[0] - center_t[0]) / z_p
            spy = center_t[1] + (wy - off_p[1] - center_t[1]) / z_p
            back = np.stack([spx - 0.5, spy - 0.5], axis=-1)

        out.append({
            "lr": lr.astype(np.float32),
            "gt": gt.astype(np.float32),
            "back": None if back is None else back.astype(np.float32),
            "wh": wh, "hh": hh,
        })
    return out


SCENES = ["pan", "zoom", "disocclusion"]
