# CSR and temporal upscaling — a measured feasibility assessment

**Question asked:** can CSR be upgraded (a "CSR 1.1") to equal DLSS 2 / 2.1?

**Answer, measured:** no — not as anything that runs from outside a game. This
document puts numbers on why, so the claim is checkable rather than asserted.

## The two kinds of upscaler

CSR is **spatial**: it resolves one finished frame (FSR 1's EASU + RCAS), needs
no engine data, and has no memory of previous frames. DLSS 2 / 2.1 and FSR 2 are
**temporal**: they accumulate detail across many frames, and to do that they need
per-frame **motion vectors, depth and sub-pixel jitter** that only the game's
renderer produces. DLSS adds a **trained neural network** on top of that
temporal machinery. Those are the two ingredients CSR does not have, and one of
them (the engine's motion vectors) is the one that decides everything below.

## What we measured

`csr/ref/` contains a small, deterministic experiment. Synthetic moving scenes
are authored analytically, so we know the **exact** motion of every pixel — which
lets us measure the one thing a real capture never lets you see: the difference
between having *perfect* motion vectors and having to *estimate* motion from the
finished frames.

Four upscalers are scored (PSNR/SSIM) against the per-frame high-res ground truth:

| | what it is |
|---|---|
| **bilinear** | the floor |
| **CSR 1.0** | our spatial upscaler, per frame, no history |
| **temporal (true motion)** | accumulate CSR across frames using **perfect** motion vectors — the honest ceiling for a *non-neural* temporal path |
| **temporal (estimated motion)** | the same, but motion is **estimated from the finished frames only** — what a capture-only external tool can actually get |

Reproduce:

```bash
cd csr/ref
pip install -r ../../tools/upscaler-ref/requirements.txt
python3 temporal_eval.py
```

## Results

128×72 → 256×144, scored over frames 8–19 (after the history warms up):

| scene | bilinear | CSR 1.0 | temporal (true) | temporal (est.) |
|---|---|---|---|---|
| pan | 23.52 | 22.66 | **28.23** | 22.16 |
| zoom | 24.85 | 23.73 | **29.32** | 19.93 |
| disocclusion | 23.77 | 22.91 | **24.21** | 20.27 |
| **mean** | 24.04 | 23.10 | **27.25** | 20.78 |

(dB PSNR. Mean SSIM: bilinear 0.907, CSR 1.0 0.893, temporal-true 0.953,
temporal-est 0.852.)

Two gaps matter:

- **temporal(true) − CSR 1.0 = +4.15 dB** (SSIM +0.060). With perfect motion
  vectors, going temporal is a large, real win. This is *why* DLSS and FSR 2 beat
  a spatial upscaler — and it is achieved here with **no neural network at all**.
- **temporal(true) − temporal(est) = +6.47 dB.** Estimating motion from finished
  frames throws away more than the entire temporal benefit. **temporal(est) lands
  2.3 dB below plain spatial CSR** — it is a net *loss*, not a gain.

## Why estimated motion collapses

- **Panning** is the easy case, and even there the estimate is corrupted: the
  sub-pixel jitter that makes temporal accumulation work is indistinguishable,
  from the outside, from real motion, so the per-frame estimate is noisy and
  temporal(est) barely matches spatial CSR instead of gaining +5.6 dB.
- **Zoom** cannot be described by the global translation an external estimator
  can cheaply recover; the misalignment grows toward the frame edges and history
  smears — temporal(est) drops to 19.93 dB, below bilinear.
- **Disocclusion** is fatal by construction: where a moving object reveals fresh
  background, there is no correct history to reproject. A perfect-motion pipeline
  still only gains +1.3 dB here; an estimating one ghosts.

The neighborhood clamp (the rectification FSR 2 / DLSS use) limits the damage but
cannot invent the motion vectors that were never available.

## The conclusion, stated plainly

1. **CSR 1.1 cannot equal DLSS 2 / 2.1 as a generic external tool.** The measured
   ceiling for the achievable-from-outside path (estimated motion) is *below*
   CSR 1.0's own spatial output. There is no tuning that closes a negative gap.
2. **Even with perfect motion vectors, the non-neural ceiling here is FSR 2-class,
   not DLSS-class.** DLSS's advantage over FSR 2 is its trained network; that is a
   separate ingredient, not runnable as "a simple script" and not shippable inside
   the self-contained binary. So "= DLSS" needs *both* engine motion vectors *and*
   a trained model.
3. **The only route to temporal quality is engine-supplied motion.** That is
   exactly categories 1–3 of [`../docs/INJECTION_LIMITATIONS.md`](../docs/INJECTION_LIMITATIONS.md)
   (native support, a mod, or a public DLSS/FSR 2/XeSS wrapper the game feeds its
   motion vectors) — and in those cases a real temporal upscaler already exists,
   so re-deriving one as "CSR" adds nothing. The capture path in
   [`CAPTURE.md`](CAPTURE.md) gives a finished frame and no motion vectors, which
   is precisely the case the numbers above show is a net loss.

## What a "CSR 1.1" can honestly be

A better and better-characterised **spatial** upscaler — sharper edge
reconstruction, smarter adaptive detail, tighter ringing control — with measured
PSNR/SSIM gains over CSR 1.0, clearly labelled "better spatial." It must never be
presented as temporal or as DLSS-equivalent. The experiment here is the standing
evidence for that boundary; rerun `temporal_eval.py` if anyone proposes to cross
it.

## Caveats on the numbers

- The scenes deliberately contain fine detail near the sampling limit, where a
  sharpener can score slightly below bilinear on MSE (the same effect documented
  for `soft_detail` in [`README.md`](README.md)); that is why CSR 1.0 sits a hair
  under bilinear here. It does not affect the temporal comparison, which measures
  accumulation on top of the *same* CSR output.
- `temporal.py` is the FSR 2 / DLSS *skeleton without the network*
  (reproject → clamp → accumulate). It is a measurement reference, not a shipping
  engine. A trained network would raise the true-motion ceiling further — and
  would still need the engine's motion vectors, so it does not change conclusion 1.
