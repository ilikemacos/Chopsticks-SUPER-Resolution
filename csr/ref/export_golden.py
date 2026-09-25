"""Emit golden vectors for the C# and HLSL ports of CSR.

Only the validated shipping configuration is exported. The rejected ablation
paths (structure tensor, Keys kernel, linear anisotropy) are not ported, so they
are deliberately not covered here.

Run: python3 export_golden.py
"""
from __future__ import annotations

import json
import pathlib

import numpy as np

from csr import CsrConfig, csr_resolve, csr_sharpen

OUT = pathlib.Path(__file__).parent / "golden"
TOLERANCE = 1.0 / 255.0   # ports must agree within one 8-bit step
CFG = CsrConfig()         # the validated configuration

CASES = [
    ("edge_vertical_2x", (8, 8), (16, 16)),
    ("edge_diagonal_1_5x", (12, 12), (18, 18)),
    ("gradient_1_333x", (12, 9), (16, 12)),
    ("checker_2x", (8, 8), (16, 16)),
    ("noise_1_333x", (12, 12), (16, 16)),
    ("colour_bands_1_333x", (12, 12), (16, 16)),
    ("soft_midtone_1_5x", (10, 10), (15, 15)),
    # These two exist because mutation testing showed that removing the
    # deringing clamp did not fail any other case. On binary 0/1 content the
    # final [0,1] clip masks overshoot; only midtone content, where overshoot
    # stays inside range, actually exercises the clamp.
    ("midtone_square_1_5x", (10, 10), (15, 15)),
    ("midtone_diagonal_1_5x", (12, 12), (18, 18)),
]


def make_input(name: str, w: int, h: int) -> np.ndarray:
    y, x = np.mgrid[0:h, 0:w]
    if name.startswith("edge_vertical"):
        a = np.zeros((h, w, 3), np.float32); a[:, w // 2:] = 1.0
    elif name.startswith("edge_diagonal"):
        a = np.repeat(((x + y) > (w + h) // 2).astype(np.float32)[:, :, None], 3, 2)
    elif name.startswith("gradient"):
        xf, yf = x.astype(np.float32), y.astype(np.float32)
        a = np.stack([xf / (w - 1), yf / (h - 1), (xf + yf) / (w + h - 2)], -1)
    elif name.startswith("checker"):
        a = np.repeat((((x + y) % 2) == 0).astype(np.float32)[:, :, None], 3, 2)
    elif name.startswith("colour_bands"):
        # Channels disagree, so this exercises the Rec.709 luma path.
        a = np.stack([((x // 2) % 2).astype(np.float32),
                      ((y // 3) % 2).astype(np.float32),
                      (((x + y) // 2) % 2).astype(np.float32)], -1)
    elif name.startswith("soft_midtone"):
        # Mid contrast, so adaptive sharpening is actually permitted to act.
        g = 0.5 + 0.15 * np.sin(x * 1.1) * np.cos(y * 0.9)
        a = np.stack([g, g * 0.97, g * 0.93], -1)
    elif name.startswith("midtone_square"):
        # Isolated midtone feature: the strongest deringing exercise found.
        a = np.full((h, w, 3), 0.35, np.float32)
        a[h // 2 - 1:h // 2 + 1, w // 2 - 1:w // 2 + 1] = 0.75
    elif name.startswith("midtone_diagonal"):
        # Diagonal midtone edge: exercises the direction estimator and the clamp
        # together.
        g = np.where((x + y) > (w + h) // 2 - 1, 0.72, 0.28).astype(np.float32)
        a = np.repeat(g[:, :, None], 3, axis=2)
    else:
        a = np.random.default_rng(11).random((h, w, 3))
    return np.ascontiguousarray(np.clip(a, 0, 1), dtype=np.float32)


def main() -> None:
    OUT.mkdir(exist_ok=True)
    index = []
    for name, (iw, ih), (ow, oh) in CASES:
        src = make_input(name, iw, ih)
        res = csr_resolve(src, ow, oh, CFG)
        full = csr_sharpen(res, CFG)
        payload = {
            "name": name,
            "tolerance": TOLERANCE,
            "config": {
                "taps": CFG.taps, "rec709Luma": CFG.rec709_luma,
                "adaptiveSharpen": CFG.adaptive_sharpen,
                "dering": CFG.dering, "sharpnessStops": CFG.sharpness,
            },
            "input": {"width": iw, "height": ih,
                      "rgb": [round(float(v), 7) for v in src.ravel()]},
            "resolve": {"width": ow, "height": oh,
                        "rgb": [round(float(v), 7) for v in res.ravel()]},
            "full": {"width": ow, "height": oh,
                     "rgb": [round(float(v), 7) for v in full.ravel()]},
        }
        (OUT / f"{name}.json").write_text(json.dumps(payload, indent=1))
        index.append({"name": name, "in": [iw, ih], "out": [ow, oh]})
        print(f"  {name:<24} {iw}x{ih} -> {ow}x{oh}")
    (OUT / "index.json").write_text(json.dumps(
        {"tolerance": TOLERANCE,
         "note": "Reference uses exact reciprocals; GPU ports use fast "
                 "approximations. Compare within tolerance, not bit-exactly.",
         "cases": index}, indent=1))
    print(f"\n{len(index)} cases -> {OUT}")


if __name__ == "__main__":
    main()
