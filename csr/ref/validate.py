"""CSR final validation against FSR 1, the algorithm it derives from.

Two pattern sets are needed. The greyscale binary set stresses edge
reconstruction; the colour/mid-contrast set is the only one that can test the
luma choice (channels must disagree) and contrast-limited sharpening (contrast
must leave headroom). Reporting only the first set would have hidden two of
CSR's three real gains, and reporting only the second would overstate them.

Run: python3 validate.py   (exits non-zero if CSR fails to beat FSR 1)
"""
from __future__ import annotations

import sys
import pathlib

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]
                      / "tools" / "upscaler-ref"))
from framefx_spatial import easu, rcas, bilinear      # FSR 1 baseline
from metrics import psnr, ssim
from patterns import PATTERNS, render_rgb
from patterns_color import PATTERNS_COLOR, render_rgb_color

from csr import CsrConfig, csr_resolve, csr_sharpen

PAIRS = [
    ("1080p -> 1440p", (1920, 1080), (2560, 1440)),
    ("720p -> 1080p", (1280, 720), (1920, 1080)),
]
CFG = CsrConfig()   # the validated configuration


def sets():
    for n in PATTERNS:
        yield n, render_rgb
    for n in PATTERNS_COLOR:
        yield n, render_rgb_color


def main() -> int:
    acc = {"bilinear": ([], []), "fsr1": ([], []), "csr": ([], [])}
    print(f"{'pattern':<14} {'pair':<16} {'bilinear':>16} {'FSR 1':>16} {'CSR':>16}")
    print("-" * 82)
    for label, i, o in PAIRS:
        for name, render in sets():
            src = render(name, *i)
            truth = render(name, *o)
            out = {
                "bilinear": bilinear(src, *o),
                "fsr1": rcas(easu(src, *o), 0.25),
                "csr": csr_sharpen(csr_resolve(src, *o, CFG), CFG),
            }
            line = f"{name:<14} {label:<16}"
            for k in ("bilinear", "fsr1", "csr"):
                p, s = psnr(out[k], truth), ssim(out[k], truth)
                acc[k][0].append(p); acc[k][1].append(s)
                line += f" {p:7.2f}/{s:.4f}"
            print(line)

    m = {k: (float(np.mean(v[0])), float(np.mean(v[1]))) for k, v in acc.items()}
    print("-" * 82)
    print(f"{'MEAN':<14} {'':<16}"
          + "".join(f" {m[k][0]:7.2f}/{m[k][1]:.4f}" for k in ("bilinear", "fsr1", "csr")))

    d_fsr = (m["csr"][0] - m["fsr1"][0], m["csr"][1] - m["fsr1"][1])
    d_bil = (m["csr"][0] - m["bilinear"][0], m["csr"][1] - m["bilinear"][1])
    print(f"\nCSR vs FSR 1:    {d_fsr[0]:+.2f} dB PSNR, {d_fsr[1]:+.4f} SSIM")
    print(f"CSR vs bilinear: {d_bil[0]:+.2f} dB PSNR, {d_bil[1]:+.4f} SSIM")

    if d_fsr[1] <= 0:
        print("\nGATE FAILED: CSR does not beat FSR 1; the claim is not admissible.")
        return 1
    print("\nGATE PASSED: CSR beats FSR 1 on both metrics.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
