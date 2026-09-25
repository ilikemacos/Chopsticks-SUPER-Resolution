"""Emit golden vectors for the C# and HLSL ports to be tested against.

The Python reference is the specification; ports are verified by loading these
fixtures and comparing within tolerance. Exact reciprocals here versus the GPU's
fast approximations mean bit-exactness is not the goal — TOLERANCE below is.

Run: python3 export_golden.py
Writes golden/*.json (small, text, diffable, safe to commit).
"""
from __future__ import annotations

import json
import pathlib

import numpy as np

from framefx_spatial import easu, rcas

OUT = pathlib.Path(__file__).parent / "golden"
TOLERANCE = 1.0 / 255.0  # ports must match within one 8-bit step

CASES = [
    # name, (in_w, in_h), (out_w, out_h), sharpness
    ("edge_vertical_2x", (8, 8), (16, 16), 0.25),
    ("edge_diagonal_1_5x", (12, 12), (18, 18), 0.25),
    ("gradient_1_333x", (12, 9), (16, 12), 0.25),
    ("checker_2x", (8, 8), (16, 16), 0.0),
    ("noise_1_333x", (12, 12), (16, 16), 0.5),
]


def make_input(name: str, w: int, h: int) -> np.ndarray:
    if name.startswith("edge_vertical"):
        a = np.zeros((h, w, 3), np.float32); a[:, w // 2:] = 1.0
    elif name.startswith("edge_diagonal"):
        y, x = np.mgrid[0:h, 0:w]
        a = np.repeat(((x + y) > (w + h) // 2).astype(np.float32)[:, :, None], 3, 2)
    elif name.startswith("gradient"):
        y, x = np.mgrid[0:h, 0:w].astype(np.float32)
        a = np.stack([x / (w - 1), y / (h - 1), (x + y) / (w + h - 2)], -1).astype(np.float32)
    elif name.startswith("checker"):
        y, x = np.mgrid[0:h, 0:w]
        a = np.repeat((((x + y) % 2) == 0).astype(np.float32)[:, :, None], 3, 2)
    else:
        a = np.random.default_rng(11).random((h, w, 3), dtype=np.float32)
    return np.ascontiguousarray(a, dtype=np.float32)


def main() -> None:
    OUT.mkdir(exist_ok=True)
    index = []
    for name, (iw, ih), (ow, oh) in ((c[0], c[1], c[2]) for c in CASES):
        sharp = dict((c[0], c[3]) for c in CASES)[name]
        src = make_input(name, iw, ih)
        e = easu(src, ow, oh)
        er = rcas(e, sharp)
        payload = {
            "name": name,
            "tolerance": TOLERANCE,
            "sharpnessStops": sharp,
            "input": {"width": iw, "height": ih,
                      "rgb": [round(float(v), 6) for v in src.ravel()]},
            "easu": {"width": ow, "height": oh,
                     "rgb": [round(float(v), 6) for v in e.ravel()]},
            "easuRcas": {"width": ow, "height": oh,
                         "rgb": [round(float(v), 6) for v in er.ravel()]},
        }
        path = OUT / f"{name}.json"
        path.write_text(json.dumps(payload, indent=1))
        index.append({"name": name, "file": path.name,
                      "in": [iw, ih], "out": [ow, oh], "sharpnessStops": sharp})
        print(f"wrote {path.relative_to(OUT.parent)}  ({path.stat().st_size / 1024:.1f} KiB)")
    (OUT / "index.json").write_text(json.dumps(
        {"tolerance": TOLERANCE,
         "note": "Reference uses exact reciprocals; GPU ports use fast "
                 "approximations, so compare within tolerance, not bit-exactly.",
         "cases": index}, indent=1))
    print(f"wrote golden/index.json  ({len(index)} cases)")


if __name__ == "__main__":
    main()
