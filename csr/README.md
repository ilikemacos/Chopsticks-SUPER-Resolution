# CSR — Chopsticks Super Resolution 1.0

A spatial upscaler derived from FSR 1: resolves a single finished frame, needs no
motion vectors, depth or jitter, and uses no machine learning.

**CSR 1.0 is proprietary** — see [`LICENSE`](LICENSE). It is licensed separately
from the rest of Universal FrameFX (MIT). It is derived from AMD FSR 1 (MIT), and
that attribution is retained in [`NOTICE.md`](NOTICE.md) as the MIT license
requires; a proprietary derivative is permitted only while that notice ships with
it.

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

### Tunables are already at their measured best

A later sweep of the two shipping knobs across the full pattern set:

- **Adaptive sharpening** is worth **+1.0 dB** over sharpening the whole frame
  uniformly, and is kept on.
- **Sharpness stops.** Weakening the sharpen nudges PSNR up by ~0.04 dB per step,
  monotonically toward *no* sharpening. That is the metric artifact, not a win:
  the reference frames are not sharpened, so less sharpening simply sits closer to
  them in MSE while looking softer. 0.25 stops is kept as a perceptual choice, and
  the ~0.04 dB is inside the noise. There is no genuine quality headroom in this
  knob, so the default is not chased toward the number.

The upshot: every default in `CsrConfig` is either measured to be best (taps=16,
Rec.709 luma, EASU anisotropy, deringing, adaptive sharpening) or a deliberate
perceptual choice with the trade-off stated (sharpness). "Improve the math"
was tried and the honest result is that the current configuration is already at
the measured optimum for the approaches tested.

### Confidence in the numbers

`ref/csr.py`, configured to FSR 1's choices, reproduces EASU **bit-exactly**
(max channel difference 0.0000). The baseline is therefore the real algorithm, so
the deltas above are attributable to the changes and not to implementation drift.

## CSR 1.0, CSR 1.1, and what each part can do

- **CSR 1.0** — the spatial upscaler below (resolve one finished frame; any app).
- **CSR 1.1** — adds **external frame generation** (optical-flow interpolation
  between two presented frames) plus any measured spatial gains. Frame gen is
  measured in [`FRAMEGEN.md`](FRAMEGEN.md): +6 dB over motion-blind blending,
  essentially at the perfect-motion ceiling — because interpolation is *bounded
  between two real frames*. It raises displayed FPS but **adds latency** and
  smears on disocclusion/HUD, so it is smoothness, never free performance.
- **Not** DLSS/FSR 2 temporal upscaling (measured impossible externally,
  [`TEMPORAL.md`](TEMPORAL.md)) and **not** engine-integrated frame generation.

The three references — `ref/csr.py`, `ref/temporal.py`, `ref/framegen.py`, each
with a self-gating eval — are the standing evidence for the boundaries above.

## Honest scope

- CSR is **better than FSR 1 by +0.74 dB** on edge reconstruction, and by
  +5.88 dB on mid-contrast detail. Real, measured, reproducible — and not a
  generational leap. It must not be marketed as one.
- On already-smooth content a sharpener loses to plain bilinear on fidelity
  metrics. CSR is an upscaler for detailed content, not a universal improvement.
- It is **clearly better than bilinear**, which is the floor.
- It is **not comparable to FSR 2, DLSS or XeSS**, and cannot be made so from
  outside a game. Those reconstruct from frame history using engine motion
  vectors; CSR has one frame. This is measured, not asserted — see
  [`TEMPORAL.md`](TEMPORAL.md): with *perfect* motion vectors a temporal path
  gains +4.15 dB over CSR 1.0, but with motion *estimated* from finished frames
  (all an external tool can get) it lands 2.3 dB **below** spatial CSR. A
  "CSR 1.1 = DLSS 2" is not achievable; run `ref/temporal_eval.py` before
  proposing otherwise.
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


## Performance

The CPU reference and the shipped file upscaler (`ufx-upscale`) resolve and
sharpen row-by-row, and both passes are parallelised across cores. The split is
deterministic — each thread owns a disjoint band of output rows — so the result
is **bit-identical to the serial output regardless of thread count**, which is
why the golden vectors still hold on a multi-core machine
(`tests/test_csr.cpp::OutputIsIdenticalRegardlessOfThreadCount` guards this).

Measured, 1080p → 4K (`--quality Performance`), 4 cores, PNG decode/encode
included: **3.13 s → 1.77 s** (~1.8×). `CSR_THREADS` caps the count (`1` forces
serial; unset = auto), and images below ~64k output pixels stay single-threaded
where thread setup would cost more than it saves.

The real-time path is the GPU compute shader, not this CPU code; this speed-up is
for offline file upscaling and for the app's preview.

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

## C# port

`src/Csr.Core` — the upscaler, `net8.0` with no Windows dependency, so it builds
and tests on Linux CI. Verified against the Python reference: **24/24 tests**,
7 golden cases × (resolve, full pipeline) plus 10 structural invariants, all
within 1/255.

The verification was mutation-tested: changing one luma weight from `0.2126f` to
`0.5f` fails 7 tests. A test suite that cannot fail proves nothing, so this was
checked rather than assumed.

`src/Csr.Capture` — Windows Graphics Capture plus the D3D11 compute dispatch.
Compiles (Windows-targeted, built on Linux), **not yet run on hardware**.
`Shaders/*.hlsl` are `cs_5_0` so feature-level 11_0 GPUs can run them; they have
not been compiled or executed. See `CAPTURE.md` for the latency and performance
analysis, including why "zero compositor lag" is not achievable on a capture path.

```bash
dotnet test csr/Csr.sln -c Release      # 24 tests, no GPU needed
cd csr/ref && python3 export_golden.py  # regenerate the golden vectors
```

The options rejected by ablation are **absent** from the C# port rather than
present-and-disabled. They measured worse than FSR 1; leaving switches for them
would only invite someone to turn them back on.

## Shader verification

The HLSL is not just compiled — it is **executed and diffed** against the same
golden vectors as the C# port. `tools/shader-verify` compiles it to SPIR-V with
glslang and runs it on Mesa's llvmpipe software Vulkan device, so it needs no GPU
and works in CI.

**All 9 cases × 2 passes match, worst deviation 4.4e-6** against a 1/255
tolerance — float-precision agreement between HLSL, Python and C#.

```bash
cd tools/shader-verify && ./run.sh
```

Still unverified, and stated as such: that `fxc` compiles the same source to
`cs_5_0` (needs Windows), and that a real GPU's `rcp`/`rsqrt` approximations stay
inside tolerance (needs hardware). See `tools/shader-verify/README.md`.
