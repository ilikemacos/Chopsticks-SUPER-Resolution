# CSR — attribution and licensing

CSR (Chopsticks Super Resolution) is derived from **AMD FidelityFX Super
Resolution 1** (EASU/RCAS), published by AMD under the **MIT licence**.

MIT permits proprietary derivatives, including closed-source ones, **provided the
copyright notice and permission notice are retained**. So "proprietary CSR based
on FSR 1" is legally sound — but the notice is not optional.

**Before CSR ships in any form, obtain the licence text for the exact FSR version
referenced and reproduce its copyright notice here verbatim.** Do not rely on
this file's summary.

Note that the surrounding repository is MIT-licensed. If CSR is to be
proprietary, its licensing needs to be stated explicitly and separated from the
repository default, or the two will contradict each other.

## Naming

CSR must not be presented as "FSR", "FidelityFX", or as an AMD product, and must
not imply AMD endorsement. Describing it as *derived from FSR 1* is accurate and
is the correct phrasing.

## What CSR is not

CSR is a **spatial** upscaler: it resolves a single finished frame.

It is not FSR 2, DLSS or XeSS, and cannot be made equivalent to them. Those are
*temporal* — their quality comes from motion vectors, a depth buffer and
sub-pixel jitter supplied by the renderer every frame. None of that is obtainable
from outside the rendering process, so this is a property of the problem, not a
limit of the implementation.
