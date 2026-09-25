# CSR — attribution and licensing

CSR (Chopsticks Super Resolution) is derived from **AMD FidelityFX Super
Resolution 1** (EASU/RCAS), published by AMD under the **MIT licence**.

MIT permits proprietary derivatives, including closed-source ones, **provided the
copyright notice and permission notice are retained**. So "proprietary CSR based
on FSR 1" is legally sound — but the notice is not optional.

## Decision (CSR 1.0)

CSR 1.0 is **proprietary** — see `csr/LICENSE`. It is licensed separately from
the surrounding repository (which remains MIT). The FSR 1 MIT notice below is
retained as MIT requires, and ships with CSR in every form.

## AMD FidelityFX Super Resolution 1 — MIT license

> Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

**Verify this against the exact FSR version you reference** (the copyright year
and holder are AMD's standard FidelityFX notice; confirm they match the release
you derived from before shipping).

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
