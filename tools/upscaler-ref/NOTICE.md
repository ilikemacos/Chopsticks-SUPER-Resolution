# Attribution

The edge-adaptive spatial upsampling (EASU) and robust contrast-adaptive
sharpening (RCAS) algorithms implemented in `framefx_spatial.py` are derived
from **AMD FidelityFX Super Resolution 1**, published by AMD under the MIT
licence.

**Before this ships, verify the exact licence text of the specific FSR version
referenced and reproduce its copyright notice here verbatim.** Do not take the
licence on trust from this file.

## Naming

This feature is called **FrameFX Spatial**. It must never be presented as "FSR",
"FSR 1", "FSR 2" or "FidelityFX" in the UI, documentation or the website, and
must never imply AMD endorsement. Per `CONTRIBUTING.md`, no proprietary vendor
binaries are redistributed — this is an independent implementation of a published
algorithm, not a repackaged vendor library.

## What this is not

This is a **spatial** upscaler. It reconstructs from a single finished frame.

It is not FSR 2, DLSS, XeSS or any other temporal upscaler, and cannot be made
equivalent to one. Those derive their quality from motion vectors, a depth buffer
and sub-pixel jitter supplied by the game's renderer each frame. None of that is
available from outside the process, so no amount of work on this code closes that
gap. See `docs/INJECTION_LIMITATIONS.md`.
