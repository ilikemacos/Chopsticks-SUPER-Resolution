"""Focused sweep after the first ablation, which showed the Keys cubic kernel is
a real win (+1.64 dB) while the structure tensor and the timid anisotropy were
losing ground. This tests CSR built on FSR 1's proven edge estimator and
anisotropy, keeping only the changes the data supported.
"""
from __future__ import annotations
import sys, pathlib, itertools
import numpy as np
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "tools" / "upscaler-ref"))
from framefx_spatial import easu, rcas, bilinear
from metrics import psnr, ssim
from patterns import PATTERNS, render_rgb
from csr import CsrConfig, csr_resolve, csr_sharpen

IN, OUT = (960, 540), (1280, 720)

CACHE = {}
def data(name):
    if name not in CACHE:
        CACHE[name] = (render_rgb(name, *IN), render_rgb(name, *OUT))
    return CACHE[name]

def score(fn):
    ps, ss = [], []
    for n in PATTERNS:
        src, truth = data(n)
        o = fn(src)
        ps.append(psnr(o, truth)); ss.append(ssim(o, truth))
    return float(np.mean(ps)), float(np.mean(ss))

fsr = score(lambda s: rcas(easu(s, *OUT), 0.25))
bil = score(lambda s: bilinear(s, *OUT))
print(f"bilinear                     {bil[0]:7.2f} dB  {bil[1]:.4f}")
print(f"FSR 1 EASU+RCAS (baseline)   {fsr[0]:7.2f} dB  {fsr[1]:.4f}\n")

print(f"{'CSR variant':<46} {'PSNR':>8} {'SSIM':>8} {'vs FSR1':>17}")
print("-" * 82)
best = None
for taps, keys_c in itertools.product((12, 16), (0.35, 0.50, 0.60, 0.75)):
    cfg = CsrConfig(taps=taps, tensor_dir=False, aniso="easu",
                    keys_kernel=True, keys_c=keys_c,
                    adaptive_sharpen=False, dering=True, sharpness=0.25)
    p, s = score(lambda x, c=cfg: csr_sharpen(csr_resolve(x, *OUT, c), c))
    lbl = f"quadrant dir, {taps} taps, Keys C={keys_c}"
    flag = ""
    if best is None or s > best[0]:
        best = (s, p, lbl, cfg); flag = "  *"
    print(f"{lbl:<46} {p:8.2f} {s:8.4f} "
          f"{p - fsr[0]:+7.2f} dB {s - fsr[1]:+.4f}{flag}")
print("-" * 82)
print(f"\nbest: {best[2]}")
print(f"  vs FSR 1:    {best[1]-fsr[0]:+.2f} dB, {best[0]-fsr[1]:+.4f} SSIM")
print(f"  vs bilinear: {best[1]-bil[0]:+.2f} dB, {best[0]-bil[1]:+.4f} SSIM")
