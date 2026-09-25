"""
CSR — Chopsticks Super Resolution. Reference implementation.

A spatial upscaler in the FSR 1 lineage: it resolves a single finished frame,
needs no motion vectors, depth or jitter, and uses no machine learning.

Where it departs from FSR 1's EASU, and why:

  1. Structure-tensor edge estimation instead of four independent '+' difference
     quadrants. The tensor yields a true dominant orientation and a principled
     anisotropy measure (coherence), rather than a heuristic "length".
  2. Full 4x4 (16 tap) support instead of 12 taps. The dropped corners are
     exactly the samples that carry diagonal-edge information.
  3. A Keys cubic radial kernel instead of the polynomial lobe approximation.
     Piecewise cubic, no trig, still cheap on a GPU, with a real negative lobe.
  4. Rec.709 luma for edge detection instead of the cheap 0.5B+0.5R+G proxy.
  5. Variance-adaptive sharpening.

Every one of those is a hypothesis. `ablate.py` measures each independently and
the README records which ones actually earned their place — options that did not
help are switched off by default, not quietly retained.

Derived from AMD FidelityFX Super Resolution 1 (MIT). See NOTICE.md: MIT permits
proprietary derivatives provided the copyright notice is retained.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

__all__ = ["CsrConfig", "csr_upscale", "csr_resolve", "csr_sharpen", "luma709"]

REC709 = np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)


@dataclass(frozen=True)
class CsrConfig:
    """Tunables. Defaults are whatever the ablation showed to be best."""
    taps: int = 16              # 12 (FSR-style) or 16 (full 4x4)
    tensor_dir: bool = False    # REJECTED by ablation: -0.38 dB vs quadrant
    rec709_luma: bool = True    # Rec.709 vs cheap 0.5B+0.5R+G
    keys_kernel: bool = False   # REJECTED by ablation: -2.13 dB vs FSR lobe
    aniso: str = "easu"         # "easu" (proven formulation) or "linear"
    stretch: float = 0.45       # "linear" only: narrowing across the edge
    along: float = 0.50         # "linear" only: widening along the edge
    keys_c: float = 0.50        # Keys C parameter; 0.5 == Catmull-Rom
    sharpness: float = 0.25     # stops; 0 is strongest
    adaptive_sharpen: bool = True
    dering: bool = True         # clamp to the 4 nearest taps — never disable


def _safe_div(n, d):
    return np.divide(n, d, out=np.zeros_like(n), where=d != 0)


def luma709(rgb: np.ndarray) -> np.ndarray:
    return rgb @ REC709


def luma_cheap(rgb: np.ndarray) -> np.ndarray:
    """FSR 1's luma-times-two proxy, kept for the ablation."""
    return 0.5 * rgb[..., 2] + (0.5 * rgb[..., 0] + rgb[..., 1])


def _gather(src, ix, iy, dx, dy):
    h, w = src.shape[:2]
    return src[np.clip(iy + dy, 0, h - 1), np.clip(ix + dx, 0, w - 1)]


def _box3(a: np.ndarray) -> np.ndarray:
    """3x3 box blur with edge replication, separable."""
    p = np.pad(a, 1, mode="edge")
    acc = np.zeros_like(a)
    for i in range(3):
        acc += p[i:i + a.shape[0], 1:1 + a.shape[1]]
    p2 = np.pad(acc / 3.0, 1, mode="edge")
    out = np.zeros_like(a)
    for i in range(3):
        out += p2[1:1 + a.shape[0], i:i + a.shape[1]]
    return out / 3.0


def _structure_tensor(L: np.ndarray):
    """Smoothed structure tensor of the source luma.

    Returns (dir_x, dir_y, coherence) on the SOURCE grid. dir points along the
    dominant gradient, i.e. across the edge, matching FSR's convention.
    """
    # Sobel gradients.
    p = np.pad(L, 1, mode="edge")
    gx = (p[0:-2, 2:] + 2 * p[1:-1, 2:] + p[2:, 2:]
          - p[0:-2, 0:-2] - 2 * p[1:-1, 0:-2] - p[2:, 0:-2]) * 0.125
    gy = (p[2:, 0:-2] + 2 * p[2:, 1:-1] + p[2:, 2:]
          - p[0:-2, 0:-2] - 2 * p[0:-2, 1:-1] - p[0:-2, 2:]) * 0.125

    Jxx = _box3(gx * gx)
    Jyy = _box3(gy * gy)
    Jxy = _box3(gx * gy)

    # Analytic eigen-decomposition of the 2x2 symmetric tensor.
    tr = Jxx + Jyy
    diff = Jxx - Jyy
    disc = np.sqrt(np.maximum(diff * diff + 4.0 * Jxy * Jxy, 0.0))
    l1 = 0.5 * (tr + disc)
    l2 = 0.5 * (tr - disc)

    # Eigenvector for the larger eigenvalue.
    vx = Jxy
    vy = l1 - Jxx
    n = np.sqrt(vx * vx + vy * vy)
    flat = n < 1e-8
    dir_x = np.where(flat, 1.0, _safe_div(vx, n)).astype(np.float32)
    dir_y = np.where(flat, 0.0, _safe_div(vy, n)).astype(np.float32)

    coh = _safe_div(l1 - l2, l1 + l2).astype(np.float32)
    coh = np.clip(coh, 0.0, 1.0)
    coh = np.where(tr < 1e-7, 0.0, coh).astype(np.float32)  # no gradient -> no edge
    return dir_x, dir_y, coh


def _keys(x: np.ndarray, C: float) -> np.ndarray:
    """Keys cubic, radial. C=0.5 is Catmull-Rom. Support is |x| < 2."""
    ax = np.abs(x)
    x2 = ax * ax
    x3 = x2 * ax
    inner = (2.0 - 1.5 * C) * x3 - (3.0 - 2.0 * C) * x2 + 1.0
    # Standard Keys outer lobe for B=0.
    outer = -C * x3 + 5.0 * C * x2 - 8.0 * C * ax + 4.0 * C
    w = np.where(ax < 1.0, inner, np.where(ax < 2.0, outer, 0.0))
    return w.astype(np.float32)


def _fsr_lobe(d2: np.ndarray, lob: np.ndarray) -> np.ndarray:
    """FSR 1's polynomial lobe, kept for the ablation."""
    wB = (2.0 / 5.0) * d2 - 1.0
    wA = lob * d2 - 1.0
    wB = wB * wB
    wA = wA * wA
    wB = (25.0 / 16.0) * wB - (25.0 / 16.0 - 1.0)
    return wB * wA


def csr_resolve(src: np.ndarray, out_w: int, out_h: int,
                cfg: CsrConfig = CsrConfig()) -> np.ndarray:
    """CSR's edge-directed resolve pass."""
    src = np.asarray(src, dtype=np.float32)
    in_h, in_w = src.shape[:2]

    L = (luma709(src) if cfg.rec709_luma else luma_cheap(src)).astype(np.float32)

    if cfg.tensor_dir:
        sdx, sdy, scoh = _structure_tensor(L)

    ox = (np.arange(out_w, dtype=np.float32) + 0.5) * (in_w / out_w) - 0.5
    oy = (np.arange(out_h, dtype=np.float32) + 0.5) * (in_h / out_h) - 0.5
    pp_x, pp_y = np.meshgrid(ox, oy)
    ip_x = np.floor(pp_x); ip_y = np.floor(pp_y)
    fx = (pp_x - ip_x).astype(np.float32)
    fy = (pp_y - ip_y).astype(np.float32)
    ix = ip_x.astype(np.int64); iy = ip_y.astype(np.int64)

    if cfg.taps == 16:
        offs = [(dx, dy) for dy in (-1, 0, 1, 2) for dx in (-1, 0, 1, 2)]
    else:
        offs = [(0, -1), (1, -1), (-1, 0), (0, 0), (1, 0), (2, 0),
                (-1, 1), (0, 1), (1, 1), (2, 1), (0, 2), (1, 2)]

    if cfg.tensor_dir:
        # Nearest-tap sample of the tensor field; it is already smoothed.
        dir_x = _gather(sdx[:, :, None], ix, iy, 0, 0)[..., 0]
        dir_y = _gather(sdy[:, :, None], ix, iy, 0, 0)[..., 0]
        coh = _gather(scoh[:, :, None], ix, iy, 0, 0)[..., 0]
    else:
        from framefx_compat import quadrant_dir_len
        dir_x, dir_y, coh = quadrant_dir_len(src, L, ix, iy, fx, fy)

    if cfg.aniso == "easu":
        # FSR 1's formulation: stretch the kernel from 1.0 on axis to sqrt(2) on
        # the diagonal, and widen it 2x along the edge at full edge strength.
        m = np.maximum(np.abs(dir_x), np.abs(dir_y))
        stretch = _safe_div(dir_x * dir_x + dir_y * dir_y, m)
        len_across = 1.0 + (stretch - 1.0) * coh
        len_along = 1.0 - 0.5 * coh
    else:
        len_across = 1.0 + cfg.stretch * coh
        len_along = 1.0 - cfg.along * coh * 0.5
    lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * coh
    clp = _safe_div(np.ones_like(lob), lob)

    acc_c = np.zeros((out_h, out_w, 3), dtype=np.float32)
    acc_w = np.zeros((out_h, out_w), dtype=np.float32)
    for dx, dy in offs:
        colour = _gather(src, ix, iy, dx, dy)
        offx = dx - fx
        offy = dy - fy
        vx = (offx * dir_x + offy * dir_y) * len_across
        vy = (offx * (-dir_y) + offy * dir_x) * len_along
        d2 = vx * vx + vy * vy
        if cfg.keys_kernel:
            w = _keys(np.sqrt(d2), cfg.keys_c)
        else:
            w = _fsr_lobe(np.minimum(d2, clp), lob)
        acc_c += colour * w[..., None]
        acc_w += w

    out = _safe_div(acc_c, acc_w[..., None])

    if cfg.dering:
        quad = np.stack([_gather(src, ix, iy, 0, 0), _gather(src, ix, iy, 1, 0),
                         _gather(src, ix, iy, 0, 1), _gather(src, ix, iy, 1, 1)], 0)
        out = np.clip(out, quad.min(0), quad.max(0))
    return np.clip(out, 0.0, 1.0).astype(np.float32)


def csr_sharpen(img: np.ndarray, cfg: CsrConfig = CsrConfig()) -> np.ndarray:
    """Contrast-limited sharpening. Cannot clip, by construction."""
    img = np.asarray(img, dtype=np.float32)
    sharp = float(np.exp2(-cfg.sharpness))

    def sh(dx, dy):
        # Edge CLAMP, not wrap. np.roll would make the top row sample the bottom
        # row, putting a wrong sharpening halo along every frame border.
        p = np.pad(img, ((1, 1), (1, 1), (0, 0)), mode="edge")
        return p[1 + dy:1 + dy + img.shape[0], 1 + dx:1 + dx + img.shape[1]]

    b, d, e, f, h = sh(0, -1), sh(-1, 0), img, sh(1, 0), sh(0, 1)
    ring = np.stack([b, d, f, h], 0)
    mn4 = ring.min(0); mx4 = ring.max(0)

    hit_min = _safe_div(mn4, 4.0 * mx4)
    hit_max = _safe_div(1.0 - mx4, 4.0 * mn4 - 4.0)
    lobe = np.maximum(-hit_min, hit_max).max(axis=2)
    lobe = np.maximum(-(0.25 - 1.0 / 16.0), np.minimum(lobe, 0.0)) * sharp

    if cfg.adaptive_sharpen:
        # Back off in low-variance regions, where sharpening only lifts noise.
        lum = luma709(img)
        var = _box3(lum * lum) - _box3(lum) ** 2
        lobe = lobe * np.clip(var * 400.0, 0.0, 1.0).astype(np.float32)

    lobe = lobe[..., None]
    out = (lobe * b + lobe * d + lobe * f + lobe * h + e) / (4.0 * lobe + 1.0)
    return np.clip(out, 0.0, 1.0).astype(np.float32)


def csr_upscale(src: np.ndarray, out_w: int, out_h: int,
                cfg: CsrConfig = CsrConfig()) -> np.ndarray:
    """Full CSR pipeline. Bypasses entirely at 1:1 — the resolve is a filter,
    not a pass-through, so running it at 1.0x would alter a native frame."""
    if (out_w, out_h) == (src.shape[1], src.shape[0]):
        return np.asarray(src, dtype=np.float32).copy()
    return csr_sharpen(csr_resolve(src, out_w, out_h, cfg), cfg)
