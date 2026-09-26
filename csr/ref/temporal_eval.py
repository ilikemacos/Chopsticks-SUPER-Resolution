"""Measure the temporal gap honestly (see csr/TEMPORAL.md).

For each synthetic moving scene, score four upscalers against the per-frame HR
ground truth:

  bilinear          -- the floor.
  CSR 1.0           -- our spatial upscaler, per frame, no history.
  temporal (true)   -- accumulate CSR across frames with PERFECT engine motion
                       vectors. The honest ceiling for a non-neural temporal path.
  temporal (est.)   -- same, but motion ESTIMATED from finished frames only, i.e.
                       what a capture-only external tool could actually get.

The two gaps that matter:
  temporal(true) - CSR 1.0        how much a temporal path can add at best.
  temporal(true) - temporal(est)  how much of that is lost when motion must be
                                  estimated from the outside.

Run: python3 temporal_eval.py   (exits non-zero if the model misbehaves)
"""
from __future__ import annotations

import sys
import pathlib

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]
                      / "tools" / "upscaler-ref"))
from framefx_spatial import bilinear            # noqa: E402
from metrics import psnr, ssim                  # noqa: E402

from csr import csr_upscale, CsrConfig          # noqa: E402
from scenes_motion import make_scene, SCENES    # noqa: E402
from temporal import reconstruct                # noqa: E402

WL, HL, RATIO, FRAMES, WARM = 128, 72, 2.0, 20, 8
CFG = CsrConfig()


def _score(pred_frames, scene, warm):
    ps, ss = [], []
    for i in range(warm, len(scene)):
        gt = scene[i]["gt"].astype(np.float64)
        pr = np.clip(pred_frames[i], 0.0, 1.0)
        ps.append(psnr(pr, gt))
        ss.append(ssim(pr, gt))
    return float(np.mean(ps)), float(np.mean(ss))


def run():
    rows = {}
    for name in SCENES:
        scene = make_scene(name, WL, HL, RATIO, FRAMES)
        bl = [bilinear(f["lr"].astype(np.float64), f["wh"], f["hh"]) for f in scene]
        sp = [csr_upscale(f["lr"].astype(np.float64), f["wh"], f["hh"], CFG)
              for f in scene]
        tt = reconstruct(scene, mode="true", cfg=CFG)
        te = reconstruct(scene, mode="estimated", cfg=CFG)
        rows[name] = {
            "bilinear": _score(bl, scene, WARM),
            "csr": _score(sp, scene, WARM),
            "t_true": _score(tt, scene, WARM),
            "t_est": _score(te, scene, WARM),
        }

    label = {"bilinear": "bilinear", "csr": "CSR 1.0 (spatial)",
             "t_true": "temporal (true motion)",
             "t_est": "temporal (estimated motion)"}
    print(f"Scored frames {WARM}..{FRAMES - 1}, {WL}x{HL} -> "
          f"{int(WL * RATIO)}x{int(HL * RATIO)}\n")
    for name in SCENES:
        r = rows[name]
        print(f"[{name}]")
        for k in ("bilinear", "csr", "t_true", "t_est"):
            print(f"  {label[k]:<28} {r[k][0]:6.2f} dB   SSIM {r[k][1]:.4f}")
        print(f"  gap temporal(true) - CSR 1.0        "
              f"{r['t_true'][0] - r['csr'][0]:+.2f} dB")
        print(f"  gap temporal(true) - temporal(est)  "
              f"{r['t_true'][0] - r['t_est'][0]:+.2f} dB")
        print()

    # Means across scenes.
    def mean(k, j):
        return float(np.mean([rows[n][k][j] for n in SCENES]))
    print("[mean over scenes]")
    for k in ("bilinear", "csr", "t_true", "t_est"):
        print(f"  {label[k]:<28} {mean(k, 0):6.2f} dB   SSIM {mean(k, 1):.4f}")

    # Honesty gates: the model must behave the way the report claims.
    ok = True
    if not (mean("t_true", 0) > mean("csr", 0) + 0.10):
        print("\nFAIL: temporal(true) does not beat CSR 1.0 -- accumulation "
              "is not helping; the model or scenes are wrong.")
        ok = False
    if not (mean("t_true", 0) - mean("t_est", 0) > 0.10):
        print("\nFAIL: estimated motion matches perfect motion -- unrealistic; "
              "the external penalty should be visible.")
        ok = False
    # On the disocclusion scene, estimated motion must not beat true motion.
    d = rows["disocclusion"]
    if d["t_est"][0] > d["t_true"][0] + 1e-6:
        print("\nFAIL: estimated beats true motion on disocclusion -- impossible "
              "if the reprojection is correct.")
        ok = False
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
