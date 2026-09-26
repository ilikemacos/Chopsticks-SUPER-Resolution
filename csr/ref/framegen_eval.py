"""Measure external frame generation honestly (see csr/FRAMEGEN.md).

Frame gen runs on the frames the app actually presents, so we interpolate
between ground-truth HR frames (isolating the interpolator from the upscaler):
take real frames t and t+2, synthesize t+1, score against the real t+1.

  naive blend           -- 0.5*A + 0.5*B, no motion. The floor.
  interp (true motion)  -- exact mid-frame reprojection. The ceiling.
  interp (est. motion)  -- flow estimated from the two frames alone. What a
                           capture-only external tool actually gets.

The honest thesis (the mirror of temporal_eval): here estimated motion is a real
WIN over the floor and lands close to the ceiling, because interpolation is
bounded between two real frames. Run: python3 framegen_eval.py
"""
from __future__ import annotations

import sys
import pathlib

import numpy as np

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]
                      / "tools" / "upscaler-ref"))
from metrics import psnr, ssim                          # noqa: E402

from scenes_motion import make_scene, backward_coords, SCENES   # noqa: E402
from framegen import interpolate, naive_blend                   # noqa: E402

WL, HL, RATIO, FRAMES, STEP = 128, 72, 2.0, 20, 2


def _mean_scores(preds, gts):
    ps = [psnr(np.clip(p, 0, 1), g) for p, g in zip(preds, gts)]
    ss = [ssim(np.clip(p, 0, 1), g) for p, g in zip(preds, gts)]
    return float(np.mean(ps)), float(np.mean(ss))


def run():
    rows = {}
    for name in SCENES:
        scene = make_scene(name, WL, HL, RATIO, FRAMES)
        gts, naive, itrue, iest = [], [], [], []
        for i in range(2, FRAMES - 2, STEP):
            a = scene[i]["gt"].astype(np.float64)
            b = scene[i + 2]["gt"].astype(np.float64)
            tgt = scene[i + 1]["gt"].astype(np.float64)
            ta = backward_coords(name, WL, HL, RATIO, i + 1, i)
            tb = backward_coords(name, WL, HL, RATIO, i + 1, i + 2)
            gts.append(tgt)
            naive.append(naive_blend(a, b))
            itrue.append(interpolate(a, b, "true", true_a=ta, true_b=tb))
            iest.append(interpolate(a, b, "estimated"))
        rows[name] = {
            "naive": _mean_scores(naive, gts),
            "true": _mean_scores(itrue, gts),
            "est": _mean_scores(iest, gts),
        }

    label = {"naive": "naive blend (no motion)",
             "true": "interp (true motion)",
             "est": "interp (estimated motion)"}
    print(f"Interpolate frame t+1 from t and t+2, {int(WL * RATIO)}x{int(HL * RATIO)}\n")
    for name in SCENES:
        r = rows[name]
        print(f"[{name}]")
        for k in ("naive", "true", "est"):
            print(f"  {label[k]:<28} {r[k][0]:6.2f} dB   SSIM {r[k][1]:.4f}")
        print(f"  gain interp(est) - naive            {r['est'][0] - r['naive'][0]:+.2f} dB")
        print(f"  gap  interp(true) - interp(est)     {r['true'][0] - r['est'][0]:+.2f} dB")
        print()

    def mean(k, j):
        return float(np.mean([rows[n][k][j] for n in SCENES]))
    print("[mean over scenes]")
    for k in ("naive", "true", "est"):
        print(f"  {label[k]:<28} {mean(k, 0):6.2f} dB   SSIM {mean(k, 1):.4f}")

    ok = True
    # Frame gen must be a real, positive external capability on every scene.
    for name in SCENES:
        r = rows[name]
        if not (r["est"][0] > r["naive"][0] + 0.25):
            print(f"\nFAIL[{name}]: estimated interpolation does not beat naive "
                  f"blend -- the flow estimator is not helping.")
            ok = False
    # Ceiling check only where the true field is complete. backward_coords encodes
    # the camera/background motion; the disocclusion scene adds an independently
    # moving occluder, so its "true" is background-only and the revealed regions
    # are ill-posed for any single-motion warp -- there true ~ est ~ naive, which
    # is itself the honest lesson, not a ceiling to enforce.
    for name in ("pan", "zoom"):
        r = rows[name]
        if r["est"][0] > r["true"][0] + 0.30:
            print(f"\nFAIL[{name}]: estimated beats the perfect-motion ceiling by "
                  f"more than noise -- the reprojection or estimator is wrong.")
            ok = False
    return ok


if __name__ == "__main__":
    sys.exit(0 if run() else 1)
