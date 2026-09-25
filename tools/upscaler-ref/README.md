# FrameFX Spatial — reference implementation

Python reference for the spatial upscaler: **EASU** (edge-adaptive spatial
upsampling) followed by **RCAS** (robust contrast-adaptive sharpening).

This is a **development tool**. It is the executable specification that the C#
and HLSL implementations are ported from and tested against. It is never shipped
in the desktop app, which publishes as a self-contained single-file .NET binary
and must not acquire a Python dependency.

## Why Python is the specification

The maths is fiddly in the places that matter — the 12-tap kernel, the edge
direction/length estimate, the anisotropic rotation and the deringing clamp.
Getting it right once in numpy, where it can be read and probed, and then pinning
it with golden vectors, is cheaper than debugging it simultaneously in a compute
shader and a C# port.

## Usage

```bash
pip install -r requirements.txt

python3 test_invariants.py   # invariants the ports must also satisfy
python3 harness.py           # quality gate: must beat bilinear
python3 export_golden.py     # emit golden/*.json for the C#/HLSL port tests
```

## Measured results

`harness.py` renders each pattern analytically at *both* the input and the
ground-truth resolution, so the comparison is against a true reference rather
than a downsampled copy. Current results, mean over 4 patterns x 2 ratios:

| | PSNR | SSIM |
|---|---|---|
| bilinear | 27.94 dB | 0.9768 |
| EASU | 31.73 dB | 0.9905 |
| EASU + RCAS | 31.73 dB | 0.9905 |

**EASU is +3.79 dB PSNR / +0.0137 SSIM over bilinear**, consistently across all
eight combinations. That measurement is the only basis on which this may be
described as an upscaler.

## Findings worth carrying into the ports

1. **Bypass EASU entirely at ratio 1.0.** It is a filter with negative lobes,
   not a pass-through. At 1.0x it is near-identity on smooth content (max delta
   0.004) but measurably alters detailed content. A "Native / no upscaling"
   preset must skip the pass, not rely on it being neutral.
2. **RCAS is near-neutral on full-contrast content, by design.** Its limiter
   refuses any sharpening that would clip, so on a pure black/white edge it is
   exactly a no-op. It only shows measurable gain on mid-contrast content
   (x1.05 local contrast at 0 stops, falling monotonically to x1.004 at 2 stops).
   Do not gate the port on RCAS improving PSNR — sharpening can reduce PSNR
   against a reference while improving perceived sharpness.
3. **Colour space is the trap.** EASU expects perceptually encoded (sRGB/gamma)
   input. Feeding it linear light produces haloing that reads like an algorithm
   bug. `ColorSpace` helpers in the port exist for this reason.
4. **Luma is `0.5*B + 0.5*R + G`** (luma times two), the cheap multi-channel
   approximation the published algorithm uses for edge detection. The port must
   match this exactly or edge direction will diverge.
5. **The deringing clamp is mandatory.** Clamping the result to the range of the
   four nearest taps is what prevents overshoot on hard edges. It is the step
   most often dropped in naive ports; invariant 3 in `test_invariants.py` fails
   loudly without it.
6. **Exact vs approximate reciprocals.** This reference uses exact `1/x` and
   `1/sqrt(x)` where the GPU code uses fast approximations. Compare ports within
   tolerance (one 8-bit step), never bit-exactly.

## Honest scope

Spatial upscaling cannot recover detail that a temporal upscaler reconstructs
from frame history. Expect "clearly better than bilinear" — which is measured
above — and not "comparable to native, FSR 2 or DLSS". It also *costs* GPU time
and does not create frames, so no FPS claim attaches to it.
