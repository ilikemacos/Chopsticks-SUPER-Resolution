"""External frame generation reference -- optical-flow frame interpolation
(see csr/FRAMEGEN.md).

This is the one part of the upscaling/frame-gen space that genuinely works from
*outside* a game: synthesize a frame halfway between two frames that were both
actually presented. Both endpoints are real, so estimation error stays local and
bounded -- the opposite of temporal upscaling (temporal.py / TEMPORAL.md), where
estimated motion accumulates in a history buffer and is a net loss.

It is classical warp-and-blend, not a neural interpolator: estimate flow between
the two frames, sample each toward the midpoint by half the motion, and blend.

  motion="true"       -- exact mid-frame reprojection from scenes_motion (the
                         ceiling: a perfect motion field).
  motion="estimated"  -- a block Lucas-Kanade flow estimated from the two frames
                         alone (what a capture-only external tool actually has).

Honest costs (quantified in the eval, spelled out in FRAMEGEN.md): a real frame
must be held to interpolate against, so latency rises; fast motion, disocclusion
and HUD/UI produce local artifacts. It raises displayed FPS, not responsiveness.
"""
from __future__ import annotations

import numpy as np

from temporal import _warp, estimate_translation


def estimate_flow(a: np.ndarray, b: np.ndarray, block: int = 32, step: int = 16):
    """Dense-ish apparent motion of content from `a` to `b`, in pixels.

    A single global translation cannot describe zoom/rotation, so estimate a
    per-block translation (windowed Lucas-Kanade) and bilinearly upsample the
    coarse block grid to a smooth per-pixel field. Deliberately simple."""
    h, w = a.shape[:2]
    ys = list(range(0, h, step)) or [0]
    xs = list(range(0, w, step)) or [0]
    cu = np.zeros((len(ys), len(xs)))
    cv = np.zeros((len(ys), len(xs)))
    half = block // 2
    for bi, cyc in enumerate(ys):
        for bj, cxc in enumerate(xs):
            y0, y1 = max(0, cyc - half), min(h, cyc + half)
            x0, x1 = max(0, cxc - half), min(w, cxc + half)
            u, v = estimate_translation(a[y0:y1, x0:x1], b[y0:y1, x0:x1])
            cu[bi, bj], cv[bi, bj] = u, v
    # Bilinearly upsample the coarse (u,v) grids to full resolution.
    gy = (np.arange(h) / max(1, h - 1)) * (len(ys) - 1)
    gx = (np.arange(w) / max(1, w - 1)) * (len(xs) - 1)

    def up(grid):
        y0 = np.floor(gy).astype(int); y1 = np.minimum(y0 + 1, len(ys) - 1)
        x0 = np.floor(gx).astype(int); x1 = np.minimum(x0 + 1, len(xs) - 1)
        fy = (gy - y0)[:, None]; fx = (gx - x0)[None, :]
        top = grid[y0][:, x0] * (1 - fx) + grid[y0][:, x1] * fx
        bot = grid[y1][:, x0] * (1 - fx) + grid[y1][:, x1] * fx
        return top * (1 - fy) + bot * fy

    return up(cu), up(cv)


def _grid(w: int, h: int):
    return np.meshgrid(np.arange(w, dtype=np.float64), np.arange(h, dtype=np.float64))


def interpolate(a: np.ndarray, b: np.ndarray, mode: str = "true",
                true_a=None, true_b=None) -> np.ndarray:
    """Synthesize the frame halfway between real frames `a` and `b`.

    true_a / true_b: exact backward coords mapping the mid-frame's pixels into
    `a` and `b` (from scenes_motion.backward_coords), used only for mode="true".
    """
    h, w = a.shape[:2]
    if mode == "true":
        wa = _warp(a, true_a.astype(np.float64))
        wb = _warp(b, true_b.astype(np.float64))
    else:
        u, v = estimate_flow(a, b)                 # content motion a -> b
        gx, gy = _grid(w, h)
        # Content at mid pixel p came from a at p-0.5*flow and reaches b at p+0.5*flow.
        wa = _warp(a, np.stack([gx - 0.5 * u, gy - 0.5 * v], axis=-1))
        wb = _warp(b, np.stack([gx + 0.5 * u, gy + 0.5 * v], axis=-1))
    return np.clip(0.5 * wa + 0.5 * wb, 0.0, 1.0)


def naive_blend(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    """The floor: cross-fade with no motion at all (what a motion-blind FG does)."""
    return np.clip(0.5 * a + 0.5 * b, 0.0, 1.0)
