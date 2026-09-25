"""Invariants that pin the reference implementation.

Run: python3 test_invariants.py
These same assertions are what the C# and HLSL ports must satisfy.
"""
import numpy as np
from framefx_spatial import easu, rcas, upscale, bilinear

FAIL = []

def check(name, ok, detail=""):
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  — {detail}" if detail else ""))
    if not ok:
        FAIL.append(name)

# 1. Solid colour must survive exactly: catches weight-normalisation errors.
for colour in [(0.0, 0.0, 0.0), (1.0, 1.0, 1.0), (0.2, 0.6, 0.9)]:
    src = np.full((64, 64, 3), colour, dtype=np.float32)
    out = easu(src, 96, 96)
    err = float(np.abs(out - np.array(colour, np.float32)).max())
    check(f"solid colour preserved {colour}", err < 1e-5, f"max err {err:.2e}")

# 2. Solid colour through RCAS is a no-op (flat region => no sharpening).
src = np.full((32, 32, 3), 0.45, dtype=np.float32)
err = float(np.abs(rcas(src, 0.25) - 0.45).max())
check("RCAS no-op on flat region", err < 1e-5, f"max err {err:.2e}")

# 3. No overshoot on a hard edge: output must stay inside the source range.
src = np.zeros((32, 32, 3), dtype=np.float32)
src[:, 16:] = 1.0
out = easu(src, 64, 64)
check("EASU no overshoot on hard edge",
      out.min() >= -1e-6 and out.max() <= 1.0 + 1e-6,
      f"range [{out.min():.4f}, {out.max():.4f}]")

# 4. RCAS cannot clip, even on a hard edge it is sharpening.
sharp = rcas(out, 0.0)
check("RCAS cannot clip", sharp.min() >= -1e-6 and sharp.max() <= 1.0 + 1e-6,
      f"range [{sharp.min():.4f}, {sharp.max():.4f}]")

# 5. A horizontal ramp stays monotonic (no reversals introduced).
ramp = np.tile(np.linspace(0, 1, 64, dtype=np.float32)[None, :, None], (32, 1, 3))
out = easu(ramp, 128, 32)
d = np.diff(out[:, :, 0], axis=1)
check("monotonic ramp stays monotonic", d.min() >= -1e-5, f"min slope {d.min():.2e}")

# 6. EASU at 1.0x is a FILTER, not a pass-through: fx=fy=0 lands on pixel
#    centres but the 12-tap kernel still samples neighbours with a negative
#    lobe. So it is near-identity only on smooth content, and the pipeline must
#    BYPASS EASU entirely at ratio 1.0 rather than rely on it being a no-op.
y, x = np.mgrid[0:48, 0:48].astype(np.float32)
smooth = np.stack([x / 47, y / 47, (x + y) / 94], axis=-1).astype(np.float32)
err = float(np.abs(easu(smooth, 48, 48) - smooth).max())
check("1.0x is near-identity on smooth content", err < 0.01, f"max err {err:.5f}")

rng = np.random.default_rng(7)
noise = rng.random((48, 48, 3), dtype=np.float32)
out = easu(noise, 48, 48)
f, g = noise, np.roll(noise, -1, axis=1)
j = np.roll(noise, -1, axis=0)
k = np.roll(np.roll(noise, -1, 0), -1, 1)
quad = np.stack([f, g, j, k], axis=0)
inside = ((out >= quad.min(0) - 1e-6) & (out <= quad.max(0) + 1e-6)).all()
check("dering clamp holds on worst-case noise", bool(inside))

# 7. Determinism.
a = upscale(noise, 72, 72, 0.25)
b = upscale(noise, 72, 72, 0.25)
check("deterministic", np.array_equal(a, b))

# 8. Weight sum: reconstruct the normalisation implicitly via a mid-grey field
#    at many fractional offsets. If weights did not sum correctly after
#    normalisation the value would drift off 0.5.
worst = 0.0
for out_size in (65, 67, 71, 96, 101, 128):
    src = np.full((37, 37, 3), 0.5, dtype=np.float32)
    worst = max(worst, float(np.abs(easu(src, out_size, out_size) - 0.5).max()))
check("weight normalisation across fractional offsets", worst < 1e-5,
      f"worst drift {worst:.2e}")

# 9. Alpha-free shape contract.
check("output shape correct", easu(noise, 100, 60).shape == (60, 100, 3))

print()
if FAIL:
    print(f"{len(FAIL)} invariant(s) FAILED: {', '.join(FAIL)}")
    raise SystemExit(1)
print("all invariants hold")
