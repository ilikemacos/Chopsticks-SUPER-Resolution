"""Colour and mid-contrast patterns.

The greyscale binary patterns in tools/upscaler-ref cannot test two of CSR's
candidate changes: with luma-identical channels both luma formulas give the same
answer, and at full black/white contrast the sharpening limiter correctly
refuses to act. These patterns exercise both.
"""
from __future__ import annotations
import numpy as np

SS = 4


def _render_rgb(fn, w, h):
    acc = np.zeros((h, w, 3), dtype=np.float32)
    for sy in range(SS):
        for sx in range(SS):
            u = (np.arange(w, dtype=np.float32) + (sx + 0.5) / SS) / w
            v = (np.arange(h, dtype=np.float32) + (sy + 0.5) / SS) / h
            uu, vv = np.meshgrid(u, v)
            acc += fn(uu, vv).astype(np.float32)
    return acc / (SS * SS)


def chroma_edges(u, v):
    """Per-channel structure at differing angles — the channels disagree, so the
    choice of luma for edge detection actually changes the direction field."""
    out = np.empty(u.shape + (3,), dtype=np.float32)
    for c, ang in enumerate((0.3, 1.1, 2.0)):
        d = u * np.cos(ang) + v * np.sin(ang)
        out[..., c] = ((d * 22.0) % 1.0 < 0.5).astype(np.float32)
    return out


def chroma_rings(u, v):
    """Coloured rings at different radii per channel."""
    r = np.hypot(u - 0.5, v - 0.5)
    out = np.empty(u.shape + (3,), dtype=np.float32)
    for c, k in enumerate((70.0, 85.0, 100.0)):
        out[..., c] = (np.sin(r * k) > 0).astype(np.float32)
    return out


def soft_detail(u, v):
    """Mid-contrast detail around 0.5 — the range where contrast-limited
    sharpening is actually permitted to do something."""
    base = 0.5 + 0.18 * np.sin(u * 60.0) * np.cos(v * 44.0)
    fine = 0.08 * np.sin((u + v) * 150.0)
    g = np.clip(base + fine, 0.0, 1.0)
    out = np.stack([g, g * 0.95 + 0.02, g * 0.9 + 0.05], axis=-1)
    return np.clip(out, 0.0, 1.0).astype(np.float32)


PATTERNS_COLOR = {
    "chroma_edges": chroma_edges,
    "chroma_rings": chroma_rings,
    "soft_detail": soft_detail,
}


def render_rgb_color(name, w, h):
    return np.ascontiguousarray(_render_rgb(PATTERNS_COLOR[name], w, h), dtype=np.float32)
