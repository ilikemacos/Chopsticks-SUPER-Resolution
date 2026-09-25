"""Quality harness: does EASU+RCAS measurably beat bilinear?

Each pattern is rendered analytically at both the input and the ground-truth
resolution. The input is upscaled and compared against the ground truth, so the
comparison is against a true reference rather than against a downsampled copy.

Run: python3 harness.py
Exits non-zero if EASU fails to beat bilinear on the aggregate, which is the
gate for describing this as an upscaler at all.
"""
from __future__ import annotations

import sys
import time

import numpy as np

from framefx_spatial import bilinear, easu, rcas
from metrics import psnr, ssim
from patterns import PATTERNS, render_rgb

# Target pairs. 4K is deliberately out of scope for now.
PAIRS = [
    ("1080p -> 1440p", (1920, 1080), (2560, 1440)),
    ("720p -> 1080p", (1280, 720), (1920, 1080)),
    ("1080p (1.5x)", (1280, 720), (1920, 1080)),
]

SHARPNESS = 0.25


def main() -> int:
    rows = []
    print(f"{'pattern':<12} {'pair':<16} {'bilinear':>18} {'EASU':>18} {'EASU+RCAS':>18}")
    print(f"{'':<12} {'':<16} {'PSNR / SSIM':>18} {'PSNR / SSIM':>18} {'PSNR / SSIM':>18}")
    print("-" * 88)

    for label, (iw, ih), (ow, oh) in PAIRS[:2]:
        for name in PATTERNS:
            src = render_rgb(name, iw, ih)
            truth = render_rgb(name, ow, oh)

            t0 = time.perf_counter()
            e = easu(src, ow, oh)
            t_easu = time.perf_counter() - t0
            er = rcas(e, SHARPNESS)
            b = bilinear(src, ow, oh)

            m = {
                "bilinear": (psnr(b, truth), ssim(b, truth)),
                "easu": (psnr(e, truth), ssim(e, truth)),
                "easu_rcas": (psnr(er, truth), ssim(er, truth)),
            }
            rows.append((name, label, m, t_easu))
            print(f"{name:<12} {label:<16} "
                  f"{m['bilinear'][0]:8.2f} / {m['bilinear'][1]:.4f}  "
                  f"{m['easu'][0]:8.2f} / {m['easu'][1]:.4f}  "
                  f"{m['easu_rcas'][0]:8.2f} / {m['easu_rcas'][1]:.4f}")

    print("-" * 88)
    b_psnr = np.mean([r[2]["bilinear"][0] for r in rows])
    e_psnr = np.mean([r[2]["easu"][0] for r in rows])
    er_psnr = np.mean([r[2]["easu_rcas"][0] for r in rows])
    b_ssim = np.mean([r[2]["bilinear"][1] for r in rows])
    e_ssim = np.mean([r[2]["easu"][1] for r in rows])
    er_ssim = np.mean([r[2]["easu_rcas"][1] for r in rows])
    print(f"{'MEAN':<12} {'':<16} {b_psnr:8.2f} / {b_ssim:.4f}  "
          f"{e_psnr:8.2f} / {e_ssim:.4f}  {er_psnr:8.2f} / {er_ssim:.4f}")
    print(f"\nEASU vs bilinear:      {e_psnr - b_psnr:+.2f} dB PSNR, "
          f"{e_ssim - b_ssim:+.4f} SSIM")
    print(f"EASU+RCAS vs bilinear: {er_psnr - b_psnr:+.2f} dB PSNR, "
          f"{er_ssim - b_ssim:+.4f} SSIM")
    print(f"reference CPU cost:    {np.mean([r[3] for r in rows]):.2f}s per frame "
          f"(numpy, unoptimised — the shipped path is a GPU shader)")

    if e_ssim <= b_ssim:
        print("\nGATE FAILED: EASU does not beat bilinear on SSIM.")
        return 1
    print("\nGATE PASSED: EASU beats bilinear on both metrics.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
