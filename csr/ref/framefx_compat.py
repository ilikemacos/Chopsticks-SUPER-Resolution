"""FSR 1's quadrant edge estimator, exposed in CSR's interface shape.

Present only so `ablate.py` can switch CSR's edge estimation back to the
original method and measure what the structure tensor is actually worth. Not
part of the shipping algorithm.
"""
from __future__ import annotations

import numpy as np


def _safe_div(n, d):
    return np.divide(n, d, out=np.zeros_like(n), where=d != 0)


def _gather(src, ix, iy, dx, dy):
    h, w = src.shape[:2]
    return src[np.clip(iy + dy, 0, h - 1), np.clip(ix + dx, 0, w - 1)]


def _quad(lA, lB, lC, lD, lE, w, dx, dy, ln):
    dc = lD - lC; cb = lC - lB
    mx = np.maximum(np.abs(dc), np.abs(cb))
    gx = lD - lB
    dx = dx + gx * w
    lx = np.clip(_safe_div(np.abs(gx), mx), 0.0, 1.0)
    ln = ln + (lx * lx) * w
    ec = lE - lC; ca = lC - lA
    my = np.maximum(np.abs(ec), np.abs(ca))
    gy = lE - lA
    dy = dy + gy * w
    ly = np.clip(_safe_div(np.abs(gy), my), 0.0, 1.0)
    ln = ln + (ly * ly) * w
    return dx, dy, ln


def quadrant_dir_len(src, L, ix, iy, fx, fy):
    """Returns (dir_x, dir_y, edge_strength) with edge_strength in [0,1]."""
    Lc = L[:, :, None]
    g = lambda dx, dy: _gather(Lc, ix, iy, dx, dy)[..., 0]
    b, c = g(0, -1), g(1, -1)
    e, f, h_ = g(-1, 0), g(0, 0), g(2, 0)
    gg = g(1, 0)
    i, j, k, l = g(-1, 1), g(0, 1), g(1, 1), g(2, 1)
    n, o = g(0, 2), g(1, 2)

    w_f = (1 - fx) * (1 - fy); w_g = fx * (1 - fy)
    w_j = (1 - fx) * fy;       w_k = fx * fy

    dx = np.zeros_like(fx); dy = np.zeros_like(fx); ln = np.zeros_like(fx)
    dx, dy, ln = _quad(b, e, f, gg, j, w_f, dx, dy, ln)
    dx, dy, ln = _quad(c, f, gg, h_, k, w_g, dx, dy, ln)
    dx, dy, ln = _quad(f, i, j, k, n, w_j, dx, dy, ln)
    dx, dy, ln = _quad(gg, j, k, l, o, w_k, dx, dy, ln)

    d2 = dx * dx + dy * dy
    zero = d2 < (1.0 / 32768.0)
    inv = _safe_div(np.ones_like(d2), np.sqrt(d2))
    inv = np.where(zero, 1.0, inv)
    dir_x = (np.where(zero, 1.0, dx) * inv).astype(np.float32)
    dir_y = (np.where(zero, 0.0, dy) * inv).astype(np.float32)
    ln = ln * 0.5
    return dir_x, dir_y, (ln * ln).astype(np.float32)
