# CSR — Chopsticks Super Resolution

A spatial upscaler derived from FSR 1: resolves a single finished frame, needs no
motion vectors, depth or jitter, and uses no machine learning.

`ref/` holds the Python reference implementation, which is the **specification**.
The shipping C#/HLSL code will be a port verified against it. Python is a
development dependency only — the desktop app publishes as a self-contained
single-file .NET binary and must not acquire a Python runtime dependency.

## Does CSR actually beat FSR 1?

Yes — on **14 of 14** pattern/ratio combinations — but by a modest amount, and
only because three of six proposed changes survived measurement.

### Headline, 1080p→1440p and 720p→1080p

Edge-reconstruction patterns (the fair summary, n=12):

| | PSNR | SSIM |
|---|---|---|
| bilinear | 28.24 dB | 0.9769 |
| FSR 1 | 31.97 dB | 0.9907 |
| **CSR** | **32.71 dB** | **0.9921** |

**CSR vs FSR 1: +0.74 dB, +0.0014 SSIM. CSR vs bilinear: +4.48 dB, +0.0152.**

On mid-contrast detail, where adaptive sharpening is permitted to act, CSR's lead
over FSR 1 is much larger: **+5.88 dB, +0.0021 SSIM**.

One row deserves calling out rather than burying: on `soft_detail`, **bilinear
beats both upscalers** (72.65 dB vs CSR's 59.08). That pattern is smooth and
band-limited, so bilinear reconstructs it almost exactly while any sharpening
moves away from the reference. This is a real property of sharpeners, not a bug,
and it is why the all-pattern mean (which flatters bilinear to 34.58 dB) is the
wrong summary statistic. Both figures are reported here so neither misleads.

### Accepted changes

| change | gain | why it works |
|---|---|---|
| **16 taps (full 4×4)** | +0.53 dB, +0.0011 SSIM | FSR 1 drops the four corners of the 4×4 support. Those are precisely the samples carrying diagonal-edge information. |
| **Rec.709 luma** for edge detection | +0.22 dB, +0.0007 SSIM | FSR 1 uses a cheap `0.5B + 0.5R + G` proxy. Where channels disagree the proxy picks a worse edge direction. Measurable only on colour patterns — exactly 0.00 on greyscale. |
| **Variance-adaptive sharpening** | +2.57 dB, +0.0022 SSIM | Backs off in low-variance regions where sharpening only lifts noise. Measurable only on mid-contrast content — at full black/white contrast the limiter refuses to act anyway. |

### Rejected — measured, did not earn a place

| change | result | conclusion |
|---|---|---|
| **Structure-tensor edge estimation** | **−0.38 dB** | Sounds more principled than FSR's four quadrant differences, and is worse. FSR computes direction per output pixel with bilinear quadrant weights; a tensor field sampled per source tap is blockier, and box-smoothing it spreads anisotropy into regions that should stay isotropic. |
| **Keys cubic radial kernel** | **−2.13 dB** | The decisive finding. FSR's polynomial lobe couples its *radius* to edge strength — narrow on flat regions, wide on edges. A fixed-support cubic cannot, and over-smooths detail. Lower negative-lobe strength scored monotonically better, i.e. the kernel was fighting the resolve. |
| **Linear anisotropy** | worse | FSR's stretch-to-√2-on-diagonal formulation, with 2× widening along the edge, is better tuned than a naive linear ramp. |

The Keys kernel initially appeared to **help** by +1.64 dB — but only while paired
with the failing tensor estimator. Once the direction field was correct it lost by
2.13 dB. Two changes measured together can mask each other; leave-one-out
ablation against a verified baseline is what caught it. Had the first result been
taken at face value, CSR would have shipped ~2 dB *worse* than the algorithm it
claims to improve on.

### Confidence in the numbers

`ref/csr.py`, configured to FSR 1's choices, reproduces EASU **bit-exactly**
(max channel difference 0.0000). The baseline is therefore the real algorithm, so
the deltas above are attributable to the changes and not to implementation drift.

## Honest scope

- CSR is **better than FSR 1 by +0.74 dB** on edge reconstruction, and by
  +5.88 dB on mid-contrast detail. Real, measured, reproducible — and not a
  generational leap. It must not be marketed as one.
- On already-smooth content a sharpener loses to plain bilinear on fidelity
  metrics. CSR is an upscaler for detailed content, not a universal improvement.
- It is **clearly better than bilinear**, which is the floor.
- It is **not comparable to FSR 2, DLSS or XeSS**. Those reconstruct from frame
  history; CSR has one frame.
- It **costs** GPU time and does not create frames. No FPS claim attaches to it.
- 4K output is out of scope for now. Ratios beyond ~2× work arithmetically but
  fall outside the design's quality range and must not be presented as
  equivalent quality.

## Running it

```bash
pip install -r ../tools/upscaler-ref/requirements.txt
cd ref
python3 validate.py   # CSR vs FSR 1 vs bilinear, gates on beating FSR 1
python3 ablate.py     # leave-one-out contribution of each change
python3 sweep.py      # kernel parameter sweep
```

## Delivery — unresolved, and the real blocker

The requirement was "works on any Windows app, without screen recording". Those
two cannot both be satisfied. To post-process another process's frames you must
obtain them, and Windows offers exactly three routes:

1. **Capture** (Windows Graphics Capture, DXGI Desktop Duplication) — excluded by
   the requirement.
2. **In-process interception** — a `dxgi.dll`/`d3d11.dll` proxy beside the target
   executable, or a Vulkan implicit layer, wrapping `Present`. Not recording, and
   higher quality (true backbuffer, no compositor round-trip, lower latency). But
   it is **per-application file placement, not universal**, anti-cheat treats it
   as tampering, and `CONTRIBUTING.md` currently forbids injection outright.
3. **Native integration** — the application calls CSR itself. Best quality, zero
   universality.

There is no fourth route: the display path below capture (DWM, MPO, the display
driver) is not reachable from user mode. Windows' own Auto Super Resolution
achieves universality precisely by living in the driver/OS on Copilot+ hardware,
which is not a place application code can go.

The algorithm above is delivery-agnostic and stays valid whichever route is
chosen. That choice is a product and policy decision, not a technical one.
