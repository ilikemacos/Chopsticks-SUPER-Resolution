"""Analytic test patterns, rendered with supersampled antialiasing.

Patterns are evaluated as continuous functions, so the same pattern can be
rendered at both the input and the ground-truth resolution independently. That
matters: it means the quality comparison never has to downsample a reference
image, so no resampling artefact from a downsample step can contaminate the
measurement.

Everything is generated procedurally, so no copyrighted game frames are ever
committed to the repository.
"""
from __future__ import annotations

import numpy as np

SS = 4  # supersamples per axis (16 samples per pixel)


def _render(fn, w: int, h: int) -> np.ndarray:
    """Average fn(u, v) over an SS x SS grid inside each pixel."""
    acc = np.zeros((h, w), dtype=np.float32)
    for sy in range(SS):
        for sx in range(SS):
            u = (np.arange(w, dtype=np.float32) + (sx + 0.5) / SS) / w
            v = (np.arange(h, dtype=np.float32) + (sy + 0.5) / SS) / h
            uu, vv = np.meshgrid(u, v)
            acc += fn(uu, vv).astype(np.float32)
    return (acc / (SS * SS)).astype(np.float32)


def diagonals(u, v):
    """Straight edges at a spread of angles — the case EASU exists for."""
    out = np.zeros_like(u)
    for i, ang in enumerate(np.linspace(0.0, np.pi, 6, endpoint=False)):
        band = (v * 6).astype(np.int32) == i
        d = u * np.cos(ang) + v * np.sin(ang)
        out = np.where(band, ((d * 18) % 1.0 < 0.5).astype(np.float32), out)
    return out


def rings(u, v):
    """Concentric rings — curvature and ringing behaviour."""
    r = np.hypot(u - 0.5, v - 0.5)
    return (np.sin(r * 90.0) > 0).astype(np.float32)


def siemens(u, v):
    """Siemens star — the classic angular resolution target."""
    th = np.arctan2(v - 0.5, u - 0.5)
    r = np.hypot(u - 0.5, v - 0.5)
    spokes = (np.sin(th * 36.0) > 0).astype(np.float32)
    return np.where(r < 0.48, spokes, 0.5)


def thin_lines(u, v):
    """Near-single-pixel lines — worst case for any spatial filter."""
    a = ((u * 240) % 12.0) < 1.0
    b = ((v * 240) % 16.0) < 1.0
    c = (((u + v) * 170) % 14.0) < 1.0
    return (a | b | c).astype(np.float32)


PATTERNS = {
    "diagonals": diagonals,
    "rings": rings,
    "siemens": siemens,
    "thin_lines": thin_lines,
}


def render_rgb(name: str, w: int, h: int) -> np.ndarray:
    """Render a pattern as RGB. Luma-identical channels keep memory sane and
    make the edge-direction maths the thing under test."""
    g = _render(PATTERNS[name], w, h)
    return np.repeat(g[:, :, None], 3, axis=2)
