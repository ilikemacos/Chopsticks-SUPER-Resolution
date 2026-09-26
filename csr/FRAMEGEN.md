# CSR frame generation — external, measured honestly

**Question:** can we add frame generation to any app from outside it?

**Answer, measured:** yes — this is the one thing in the upscaling/frame-gen space
that genuinely works externally, and the numbers show why. But it costs latency
and it produces interpolated frames, not more real work, so it is not "free FPS."

## Two kinds of frame generation

- **Engine-integrated (FSR 3 FG, XeSS FG, DLSS 3).** The game hands the runtime
  **motion vectors, depth and a UI mask** and lets it own the swapchain. It can
  extrapolate and pair with a latency-reduction path (Reflex/anti-lag). This is
  per-game and cannot be added from outside — see
  [`../docs/HOW_FRAME_GENERATION_WORKS.md`](../docs/HOW_FRAME_GENERATION_WORKS.md).
- **External optical-flow interpolation (this — "CSR frame gen").** Take two
  frames the app already presented, estimate optical flow from the **images
  themselves**, and synthesize a frame halfway between them. No engine data. This
  is what Lossless Scaling ships, and it works on any app.

## Why external frame gen works when external *temporal upscaling* does not

[`TEMPORAL.md`](TEMPORAL.md) measured that a temporal upscaler fed motion
**estimated** from finished frames is a *net loss* — 2.3 dB below plain spatial
CSR — because the estimation error accumulates in a history buffer over many
frames. Frame interpolation is the opposite: it is **bounded between two real
frames**. Both endpoints are ground truth, the synthetic frame is shown for a few
milliseconds flanked by real ones, and error cannot accumulate. So the *same*
imperfect optical flow that sinks temporal upscaling is good enough here.

## What we measured

`csr/ref/framegen.py` is classical warp-and-blend (no neural network): estimate
flow between two frames, sample each toward the midpoint by half the motion, and
blend. `csr/ref/framegen_eval.py` interpolates frame *t+1* from real frames *t*
and *t+2* and scores it against the real *t+1*, for three methods:

| | what it is |
|---|---|
| **naive blend** | 0.5·A + 0.5·B, no motion — a motion-blind FG floor |
| **interp (true motion)** | exact mid-frame reprojection — the ceiling |
| **interp (estimated motion)** | flow estimated from the two frames only — the external reality |

Reproduce:

```bash
cd csr/ref
pip install -r ../../tools/upscaler-ref/requirements.txt
python3 framegen_eval.py
```

## Results

256×144 presented frames, interpolating the middle frame:

| scene | naive blend | interp (true) | interp (est.) |
|---|---|---|---|
| pan | 27.90 | 37.14 | **36.89** |
| zoom | 27.46 | 36.89 | **36.13** |
| disocclusion | 19.01 | 19.40 | **19.48** |
| **mean** | 24.79 | 31.14 | **30.83** |

(dB PSNR. Mean SSIM: naive 0.917, true 0.933, estimated 0.936.)

- **interp(estimated) − naive blend = +6.04 dB** (pan +8.99, zoom +8.67). Motion
  compensation is the whole game, and estimating motion from the images captures
  almost all of it.
- **interp(true) − interp(estimated) = +0.31 dB.** Estimated interpolation sits
  essentially *at* the perfect-motion ceiling — the mirror image of temporal
  upscaling, and the measured reason external frame gen is worth building.

## The honest costs — this is not free FPS

- **Latency goes up, not down.** To interpolate against frame B you must already
  have it, so a real frame is held back before the synthetic one is shown. Native
  FG offsets this with Reflex/anti-lag inside the engine; from outside another
  process there is no such lever. **Displayed FPS rises; input responsiveness gets
  worse.** The UI must say this, and never present the higher number as "more
  performance." This is the project's "no fake FPS" rule applied directly.
- **Interpolated, not rendered.** The synthetic frame is a guess between two real
  ones. It adds smoothness, not new game state or new information.
- **Artifacts on the hard cases.** The `disocclusion` row is the honest tell:
  where a moving object reveals background hidden in the other frame, no
  warp-and-blend (estimated *or* perfect-motion) reconstructs it — every method
  collapses to ~19 dB, barely above naive. Fast motion and, especially,
  **HUD/UI** smear the same way, because there is no engine data to mask the UI
  out of the flow. Best used at an already-decent base frame rate (≈60 FPS+),
  where the synthetic frame is a small step and errors are least visible.

## Where it runs

The capture → present pipeline in [`CAPTURE.md`](CAPTURE.md)
(`csr/src/Csr.Capture`) is the natural host: capture two frames, run CSR spatial
upscaling, interpolate a middle frame, and present real → synthetic → real
through `CsrPresenter`. That GPU path is written-not-run here (no GPU), the same
boundary as the rest of the capture code; this document plus `framegen.py` are
the verified reference the GPU port must match.

## What "CSR 1.1" is, then

- **CSR 1.0** — the spatial upscaler (resolves one finished frame; works on any
  app).
- **CSR 1.1** — adds **external frame generation** (this interpolation path) and
  any measured spatial-quality gains. It raises resolution *and* smoothness from
  outside any app, honestly labelled.
- It is **not** DLSS/FSR 2 temporal upscaling (measured impossible externally,
  `TEMPORAL.md`) and **not** engine-integrated frame generation. The three
  references — `ref/csr.py`, `ref/temporal.py`, `ref/framegen.py`, each with its
  eval — are the standing evidence for what each part can and cannot do.

## Caveats on the numbers

- `framegen.py` is a measurement reference (block Lucas-Kanade flow + symmetric
  warp/blend), not a shipping engine; a better flow estimator or occlusion model
  would narrow the disocclusion gap but not change the conclusions.
- `backward_coords` (the "true motion") encodes the camera/background motion. The
  disocclusion scene deliberately adds an independently moving occluder, so its
  "true" field is background-only and the revealed regions are ill-posed for any
  single-motion warp — which is exactly why true ≈ estimated ≈ naive there.
