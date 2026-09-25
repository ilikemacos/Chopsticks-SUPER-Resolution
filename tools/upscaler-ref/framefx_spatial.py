"""
FrameFX Spatial — reference implementation of edge-adaptive spatial upscaling
(EASU) and robust contrast-adaptive sharpening (RCAS).

This module is the *specification*. The C# and HLSL implementations in the
desktop app are ports, and are tested by diffing against golden vectors emitted
from here (see harness.py). It is a development tool only and is never shipped
in the application.

Algorithm derived from AMD FidelityFX Super Resolution 1 (MIT). See NOTICE.md.

Two deliberate differences from the published GPU code:

  * Exact reciprocals and inverse square roots are used where the GPU code uses
    the APrx* fast approximations. Those approximations are a speed trade-off,
    not part of the algorithm, and reproducing their exact bit patterns would
    make this a worse specification. Ports are therefore compared with a
    tolerance rather than bit-exactly.
  * Everything is float32 in [0,1] and operates on *perceptually encoded*
    (sRGB/gamma) values. EASU expects non-linear input; feeding it linear light
    produces haloing that reads like an algorithm bug but is not one.
"""

from __future__ import annotations

import numpy as np

__all__ = ["easu", "rcas", "upscale", "luma2", "bilinear"]

# Window of the RCAS negative lobe. From the published implementation.
RCAS_LIMIT = 0.25 - (1.0 / 16.0)


def _safe_div(num: np.ndarray, den: np.ndarray) -> np.ndarray:
    """num/den, defined as 0 where den == 0.

    The GPU code leans on its approximate reciprocal returning a large finite
    value here. An exact 1/0 would be inf and then 0*inf = nan, so the zero
    case is made explicit instead.
    """
    return np.divide(num, den, out=np.zeros_like(num), where=den != 0)


def luma2(rgb: np.ndarray) -> np.ndarray:
    """Luma times two: 0.5*B + 0.5*R + G.

    The cheapest usable multi-channel luma, and the one the published EASU uses
    for edge detection. The factor of two is irrelevant because every consumer
    takes ratios or differences of it.
    """
    return 0.5 * rgb[..., 2] + (0.5 * rgb[..., 0] + rgb[..., 1])


def _gather(src: np.ndarray, ix: np.ndarray, iy: np.ndarray,
            dx: int, dy: int) -> np.ndarray:
    """Fetch the tap at (ix+dx, iy+dy) with edge clamping."""
    h, w = src.shape[:2]
    x = np.clip(ix + dx, 0, w - 1)
    y = np.clip(iy + dy, 0, h - 1)
    return src[y, x]


def _set_dir_len(lA, lB, lC, lD, lE, w, dir_x, dir_y, length):
    """Accumulate gradient direction and edge length for one quadrant.

    The '+' pattern around the centre tap C:

            A
          B C D
            E
    """
    # Horizontal axis.
    dc = lD - lC
    cb = lC - lB
    len_x = np.maximum(np.abs(dc), np.abs(cb))
    dir_x_q = lD - lB
    dir_x += dir_x_q * w
    lx = np.clip(_safe_div(np.abs(dir_x_q), len_x), 0.0, 1.0)
    length += (lx * lx) * w

    # Vertical axis.
    ec = lE - lC
    ca = lC - lA
    len_y = np.maximum(np.abs(ec), np.abs(ca))
    dir_y_q = lE - lA
    dir_y += dir_y_q * w
    ly = np.clip(_safe_div(np.abs(dir_y_q), len_y), 0.0, 1.0)
    length += (ly * ly) * w

    return dir_x, dir_y, length


def _tap(acc_c, acc_w, off_x, off_y, dir_x, dir_y,
         len_x, len_y, lob, clp, colour):
    """One weighted tap, with the kernel rotated onto the edge direction."""
    # Rotate the offset into edge space.
    vx = off_x * dir_x + off_y * dir_y
    vy = off_x * (-dir_y) + off_y * dir_x
    # Anisotropy: stretch along the edge, compress across it.
    vx = vx * len_x
    vy = vy * len_y
    d2 = np.minimum(vx * vx + vy * vy, clp)

    # Approximation of a Lanczos-2 lobe without sin/sqrt:
    #   (25/16 * (2/5*x^2 - 1)^2 - (25/16 - 1)) * (lob*x^2 - 1)^2
    #   |______________________________________|  |_____________|
    #                    base                         window
    wB = (2.0 / 5.0) * d2 - 1.0
    wA = lob * d2 - 1.0
    wB = wB * wB
    wA = wA * wA
    wB = (25.0 / 16.0) * wB - (25.0 / 16.0 - 1.0)
    w = wB * wA

    acc_c += colour * w[..., None]
    acc_w += w
    return acc_c, acc_w


def easu(src: np.ndarray, out_w: int, out_h: int) -> np.ndarray:
    """Edge-adaptive spatial upsampling.

    src: (H, W, 3) float32 in [0,1], perceptually encoded.
    Returns (out_h, out_w, 3) float32.
    """
    src = np.asarray(src, dtype=np.float32)
    if src.ndim != 3 or src.shape[2] != 3:
        raise ValueError("src must be (H, W, 3)")
    in_h, in_w = src.shape[:2]

    # Centre-aligned source position for each output pixel.
    sx = in_w / out_w
    sy = in_h / out_h
    ox = np.arange(out_w, dtype=np.float32)
    oy = np.arange(out_h, dtype=np.float32)
    pp_x = (ox + 0.5) * sx - 0.5
    pp_y = (oy + 0.5) * sy - 0.5
    pp_x, pp_y = np.meshgrid(pp_x, pp_y)

    ip_x = np.floor(pp_x)
    ip_y = np.floor(pp_y)
    fx = (pp_x - ip_x).astype(np.float32)
    fy = (pp_y - ip_y).astype(np.float32)
    ix = ip_x.astype(np.int64)
    iy = ip_y.astype(np.int64)

    # The 12-tap neighbourhood: a 4x4 grid with the corners dropped.
    #
    #        b c
    #      e f g h
    #      i j k l
    #        n o
    taps = {
        "b": (0, -1), "c": (1, -1),
        "e": (-1, 0), "f": (0, 0), "g": (1, 0), "h": (2, 0),
        "i": (-1, 1), "j": (0, 1), "k": (1, 1), "l": (2, 1),
        "n": (0, 2), "o": (1, 2),
    }
    C = {k: _gather(src, ix, iy, dx, dy) for k, (dx, dy) in taps.items()}
    L = {k: luma2(v) for k, v in C.items()}

    # Bilinear weights of the four centre taps, used to blend the four
    # quadrant gradient estimates.
    w_f = (1.0 - fx) * (1.0 - fy)
    w_g = fx * (1.0 - fy)
    w_j = (1.0 - fx) * fy
    w_k = fx * fy

    dir_x = np.zeros_like(fx)
    dir_y = np.zeros_like(fx)
    length = np.zeros_like(fx)
    dir_x, dir_y, length = _set_dir_len(L["b"], L["e"], L["f"], L["g"], L["j"], w_f, dir_x, dir_y, length)
    dir_x, dir_y, length = _set_dir_len(L["c"], L["f"], L["g"], L["h"], L["k"], w_g, dir_x, dir_y, length)
    dir_x, dir_y, length = _set_dir_len(L["f"], L["i"], L["j"], L["k"], L["n"], w_j, dir_x, dir_y, length)
    dir_x, dir_y, length = _set_dir_len(L["g"], L["j"], L["k"], L["l"], L["o"], w_k, dir_x, dir_y, length)

    # Normalise the direction, treating a near-zero gradient as "no edge".
    dir2 = dir_x * dir_x + dir_y * dir_y
    zero = dir2 < (1.0 / 32768.0)
    inv = _safe_div(np.ones_like(dir2), np.sqrt(dir2))
    inv = np.where(zero, 1.0, inv)
    dir_x = np.where(zero, 1.0, dir_x) * inv
    dir_y = np.where(zero, 0.0, dir_y) * inv

    # Remap accumulated length from [0,2] to [0,1] and shape it.
    length = length * 0.5
    length = length * length

    # Stretch the kernel from 1.0 on axis to sqrt(2) on the diagonal.
    m = np.maximum(np.abs(dir_x), np.abs(dir_y))
    stretch = _safe_div(dir_x * dir_x + dir_y * dir_y, m)
    len_x = 1.0 + (stretch - 1.0) * length
    len_y = 1.0 - 0.5 * length

    # On an edge the window narrows from +/-sqrt(2) to a little beyond 2.
    lob = 0.5 + ((1.0 / 4.0 - 0.04) - 0.5) * length
    clp = _safe_div(np.ones_like(lob), lob)

    acc_c = np.zeros((out_h, out_w, 3), dtype=np.float32)
    acc_w = np.zeros((out_h, out_w), dtype=np.float32)
    for k, (dx, dy) in taps.items():
        acc_c, acc_w = _tap(acc_c, acc_w, dx - fx, dy - fy,
                            dir_x, dir_y, len_x, len_y, lob, clp, C[k])

    out = acc_c / acc_w[..., None]

    # Deringing: clamp to the range of the four nearest taps. Dropping this
    # step is the most common porting mistake and shows up as overshoot on
    # hard edges.
    quad = np.stack([C["f"], C["g"], C["j"], C["k"]], axis=0)
    lo = quad.min(axis=0)
    hi = quad.max(axis=0)
    return np.clip(out, lo, hi).astype(np.float32)


def rcas(img: np.ndarray, sharpness: float = 0.25) -> np.ndarray:
    """Robust contrast-adaptive sharpening, at output resolution.

    sharpness is in stops: 0.0 is the strongest, higher is gentler.
    The negative lobe is limited so the result cannot clip.
    """
    img = np.asarray(img, dtype=np.float32)
    sharp = float(np.exp2(-sharpness))

    def shift(dx: int, dy: int) -> np.ndarray:
        return np.roll(np.roll(img, -dy, axis=0), -dx, axis=1)

    #     b
    #   d e f
    #     h
    b, d, e, f, h = shift(0, -1), shift(-1, 0), img, shift(1, 0), shift(0, 1)

    ring = np.stack([b, d, f, h], axis=0)
    mn4 = ring.min(axis=0)
    mx4 = ring.max(axis=0)

    # Limiters, per channel: how much negative lobe fits before clipping.
    hit_min = _safe_div(mn4, 4.0 * mx4)
    hit_max = _safe_div(1.0 - mx4, 4.0 * mn4 - 4.0)
    lobe_rgb = np.maximum(-hit_min, hit_max)
    lobe = lobe_rgb.max(axis=2)
    lobe = np.maximum(-RCAS_LIMIT, np.minimum(lobe, 0.0)) * sharp
    lobe = lobe[..., None]

    out = (lobe * b + lobe * d + lobe * f + lobe * h + e) / (4.0 * lobe + 1.0)
    return np.clip(out, 0.0, 1.0).astype(np.float32)


def bilinear(src: np.ndarray, out_w: int, out_h: int) -> np.ndarray:
    """Centre-aligned bilinear resample. The baseline EASU has to beat."""
    src = np.asarray(src, dtype=np.float32)
    in_h, in_w = src.shape[:2]
    px = (np.arange(out_w, dtype=np.float32) + 0.5) * (in_w / out_w) - 0.5
    py = (np.arange(out_h, dtype=np.float32) + 0.5) * (in_h / out_h) - 0.5
    px, py = np.meshgrid(px, py)
    x0 = np.floor(px).astype(np.int64)
    y0 = np.floor(py).astype(np.int64)
    tx = (px - x0)[..., None].astype(np.float32)
    ty = (py - y0)[..., None].astype(np.float32)
    cx0 = np.clip(x0, 0, in_w - 1); cx1 = np.clip(x0 + 1, 0, in_w - 1)
    cy0 = np.clip(y0, 0, in_h - 1); cy1 = np.clip(y0 + 1, 0, in_h - 1)
    top = src[cy0, cx0] * (1 - tx) + src[cy0, cx1] * tx
    bot = src[cy1, cx0] * (1 - tx) + src[cy1, cx1] * tx
    return (top * (1 - ty) + bot * ty).astype(np.float32)


def upscale(src: np.ndarray, out_w: int, out_h: int,
            sharpness: float = 0.25) -> np.ndarray:
    """The full two-pass pipeline: EASU resolve, then RCAS sharpen."""
    return rcas(easu(src, out_w, out_h), sharpness)
