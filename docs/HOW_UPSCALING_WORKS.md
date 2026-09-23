# How upscaling works

Modern GPU upscalers render a game at a lower **internal resolution** and
reconstruct a higher **output resolution**. There are two broad families.

## Spatial upscaling (e.g. FSR 1)

A spatial upscaler works on a single finished frame. It takes the low-resolution
color image, applies edge-adaptive sharpening and scaling, and outputs a larger
image. It needs almost nothing from the engine, which is why it runs on any GPU —
but because it has no motion information, it cannot recover sub-pixel detail and
tends to look softer than temporal methods.

## Temporal upscaling (FSR 2/3, XeSS, DLSS)

Temporal upscalers accumulate detail across many frames. To do that they need,
**every frame**, from the game engine:

- **Motion vectors** — where each pixel moved since the previous frame.
- **Depth buffer** — scene depth, used to reject bad samples at edges.
- **Jitter offsets** — a sub-pixel camera offset applied per frame so that,
  over time, the samples cover the full high-resolution grid.
- Correctly-flagged **color** (pre- or post-tonemap, depending on the SDK).

The upscaler warps the previous frames using the motion vectors, aligns them to
the current frame, and blends them. This is why temporal upscaling looks far
sharper than spatial — and why it **cannot** be added from outside the game: an
external tool has no access to motion vectors or jitter, and cannot make the
engine apply the jitter in the first place.

## Quality modes and scale ratios

A quality mode is just a scale ratio between internal and output resolution:

| Mode              | Scale | 2560×1440 internal |
| ----------------- | ----- | ------------------ |
| Native / AA       | 1.0×  | 2560 × 1440        |
| Ultra Quality     | 1.3×  | 1970 × 1108        |
| Quality           | 1.5×  | 1707 × 960         |
| Balanced          | 1.7×  | 1506 × 847         |
| Performance       | 2.0×  | 1280 × 720         |
| Ultra Performance | 3.0×  | 853 × 480          |

Lower internal resolution → higher frame rate, lower image quality. Universal
FrameFX computes these numbers for you and shows them as a configuration
estimate, e.g. `Native → 1280 × 720 → 2560 × 1440`.

## What Universal FrameFX does

It does **not** run the upscaler against a live game. It:

1. Detects which upscalers your hardware can run.
2. Lets you pick a mode and computes the render resolution.
3. Writes the choice into a game's own configuration (with a backup), or copies
   a public wrapper DLL where one exists.

The actual upscaling is always performed by the game + vendor runtime.
