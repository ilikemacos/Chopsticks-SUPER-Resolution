"""Leave-one-out ablation: does each CSR change actually earn its place?

CSR claims to improve on FSR 1. That claim is only admissible with a
measurement, so every departure from FSR 1 is disabled in turn and scored. A
change that does not improve the mean is reported as such and switched off by
default rather than quietly retained.

Run: python3 ablate.py
"""
from __future__ import annotations

import sys
import pathlib
import time

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]
                      / "tools" / "upscaler-ref"))
from framefx_spatial import easu, rcas, bilinear   # FSR 1 baseline
from metrics import psnr, ssim
from patterns import PATTERNS, render_rgb

from csr import CsrConfig, csr_resolve, csr_sharpen

IN_W, IN_H = 960, 540
OUT_W, OUT_H = 1280, 720   # 1.333x, the primary target ratio


def score(fn) -> tuple[float, float]:
    ps, ss = [], []
    for name in PATTERNS:
        src = render_rgb(name, IN_W, IN_H)
        truth = render_rgb(name, OUT_W, OUT_H)
        out = fn(src)
        ps.append(psnr(out, truth))
        ss.append(ssim(out, truth))
    return float(np.mean(ps)), float(np.mean(ss))


def main() -> int:
    full = CsrConfig()

    variants: list[tuple[str, object]] = [
        ("bilinear (floor)", lambda s: bilinear(s, OUT_W, OUT_H)),
        ("FSR 1 EASU+RCAS (baseline)", lambda s: rcas(easu(s, OUT_W, OUT_H), 0.25)),
        ("CSR full", lambda s: csr_sharpen(csr_resolve(s, OUT_W, OUT_H, full), full)),
    ]

    # Leave-one-out: turn each CSR-specific change back to the FSR 1 behaviour.
    loo = {
        "  -12 taps (was 16)": dict(taps=12),
        "  -quadrant dir (no tensor)": dict(tensor_dir=False),
        "  -cheap luma (no Rec.709)": dict(rec709_luma=False),
        "  -FSR lobe (no Keys cubic)": dict(keys_kernel=False),
        "  -fixed sharpen (not adaptive)": dict(adaptive_sharpen=False),
        "  -no dering clamp": dict(dering=False),
    }
    for label, over in loo.items():
        cfg = CsrConfig(**{**full.__dict__, **over})
        variants.append((label, lambda s, c=cfg: csr_sharpen(csr_resolve(s, OUT_W, OUT_H, c), c)))

    print(f"{IN_W}x{IN_H} -> {OUT_W}x{OUT_H}, mean over {len(PATTERNS)} patterns\n")
    print(f"{'variant':<32} {'PSNR':>8} {'SSIM':>8} {'vs FSR1':>16}   {'time':>6}")
    print("-" * 78)

    results = {}
    base = None
    for label, fn in variants:
        t0 = time.perf_counter()
        p, s = score(fn)
        dt = time.perf_counter() - t0
        results[label.strip()] = (p, s)
        if label.startswith("FSR 1"):
            base = (p, s)
        delta = "" if base is None else f"{p - base[0]:+6.2f} dB {s - base[1]:+.4f}"
        print(f"{label:<32} {p:8.2f} {s:8.4f} {delta:>16}   {dt:5.1f}s")

    print("-" * 78)
    csr_p, csr_s = results["CSR full"]
    fsr_p, fsr_s = results["FSR 1 EASU+RCAS (baseline)"]
    bil_p, bil_s = results["bilinear (floor)"]
    print(f"\nCSR vs FSR 1:    {csr_p - fsr_p:+.2f} dB PSNR, {csr_s - fsr_s:+.4f} SSIM")
    print(f"CSR vs bilinear: {csr_p - bil_p:+.2f} dB PSNR, {csr_s - bil_s:+.4f} SSIM")

    print("\nPer-change contribution (removing it costs this much):")
    for label in loo:
        p, s = results[label.strip()]
        print(f"  {label.strip():<30} {csr_p - p:+6.2f} dB  {csr_s - s:+.4f}"
              f"{'   <-- no benefit, disable' if (csr_s - s) <= 0 else ''}")

    if csr_s <= fsr_s:
        print("\nGATE FAILED: CSR does not beat FSR 1. It must not be described as better.")
        return 1
    print("\nGATE PASSED: CSR beats FSR 1 on SSIM.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
