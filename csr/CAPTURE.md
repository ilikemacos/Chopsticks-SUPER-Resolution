# CSR capture path — findings

## The requirement, and which half of it survives

The ask was WGC or Desktop Duplication, ultra-low overhead, "direct hardware /
DirectX frame boundary lookup", and **zero compositor lag**.

Three of those are achievable. The fourth is not, and it is worth being exact
about why rather than shipping something that quietly adds latency.

### WGC over Desktop Duplication — settled

WGC, and not close:

| | WGC | Desktop Duplication |
|---|---|---|
| Target | a window or a monitor | a whole display output only |
| Occluded window | works | cannot |
| Window resize | `FramePool.Recreate` | reinitialise |
| Rotation / mode change | handled | you handle it |
| Intended purpose | app capture | remote desktop |

Desktop Duplication would force us to capture the entire screen and crop, and it
breaks the moment the target is partially covered. For per-application upscaling
that is disqualifying.

### "Direct hardware / DirectX frame boundary lookup" — no such API, but the
### mechanism you want does exist

There is no API that hands you a frame at a hardware boundary. What WGC gives is
better than it sounds:

```
Direct3D11CaptureFramePool.CreateFreeThreaded(...)   // pool thread, not UI thread
  -> FrameArrived
  -> TryGetNextFrame()
  -> Direct3D11CaptureFrame.Surface            (IDirect3DSurface)
  -> IDirect3DDxgiInterfaceAccess.GetInterface (ID3D11Texture2D)
```

That texture **is the VRAM allocation DWM already holds**. No staging texture,
no `Map`, no `memcpy`, no readback. The frame never touches system memory from
capture through to present. That is the entire "ultra-low overhead" claim and it
is real — see `GetTexture` in `CsrCaptureSession.cs`.

### "Zero compositor lag" — not achievable on this path

WGC delivers frames **after** DWM composition, and our upscaled output is then
presented through DWM again:

```
game Present -> DWM composes -> WGC frame -> CSR -> our Present -> DWM composes
```

That is a structural **one to two frames** (~16–33 ms at 60 Hz). No buffer count,
thread priority or flag removes it, because the composition step is the thing
producing the frame we read.

The only way to avoid the round trip is to run CSR **inside** the target process
before its `Present` — a `dxgi.dll` proxy or a Vulkan layer. That is not capture,
it is per-application file placement, it is what anti-cheat flags as tampering,
and `CONTRIBUTING.md` currently forbids it. It is a real option with real costs,
but it is a different product decision.

**So: say "adds about one frame of latency", not "zero lag".** The claim is
checkable by anyone with a high-speed camera, and being wrong about it in
marketing is the kind of thing that destroys trust in everything else we say.

## Will it run on a GeForce 700-series card or a 6 GB laptop?

First, a naming correction: there is no "GTX 7-series". Assuming **GeForce GTX
700 series** (Kepler, 2013), three things matter:

1. **Feature level.** Kepler is D3D11 FL 11_0 with compute shader 5.0. The CSR
   shaders are compiled `cs_5_0` precisely for this — shader model 6 needs DXIL,
   which Kepler has no path for. See `CompileEmbedded`.
2. **OS floor.** CSR needs Windows 10 1809 (17763) for the free-threaded frame
   pool. The analyzer caught this during the port: WGC itself shipped in 1803,
   but `CreateFreeThreaded` did not arrive until 1809.
3. **Driver support is the real problem.** NVIDIA ended Kepler feature driver
   support in 2021 (474.xx branch, security-only afterwards). A Kepler card on
   Windows 10 will run this; on current Windows 11 builds it is not a
   configuration anyone should promise.

### VRAM is a non-issue

At 1080p→1440p, B8G8R8A8:

| buffer | size |
|---|---|
| capture pool (2 × 1080p) | 16.6 MB |
| intermediate (1440p) | 14.7 MB |
| output (1440p) | 14.7 MB |
| **total** | **≈ 46 MB** |

Under 1% of a 6 GB budget. VRAM will never be the limit here.

### GPU time is the real constraint, and it must be measured

The resolve is 16 texture fetches per output pixel — at 1440p that is ~59 M
fetches, and it is fetch-bound rather than ALU-bound. On Kepler-class hardware
expect single-digit milliseconds; on anything modern, well under one.

**Those are estimates and this document will not pretend otherwise.** No such
hardware is available in this environment, so `CsrGpuPipeline` carries D3D11
timestamp-disjoint queries and exposes `LastGpuMilliseconds` — a measured number,
read non-blocking so polling it never stalls the pipeline. Measure before
claiming, and publish the measurement.

### The cost that actually matters

CSR runs on the **same GPU the game is using**, so its cost comes straight out of
the game's frame budget. On a GPU-bound title on an old card, 3 ms of CSR is 3 ms
the game does not get.

This is why CSR only makes sense when the game renders at a *lower* resolution.
At 1080p→1440p the game draws 44% fewer pixels; that saving dwarfs CSR's cost,
and the net is a clear win. Running CSR at native resolution is pure loss — which
is why `CsrUpscaler.Upscale` bypasses entirely at 1:1 instead of filtering a
frame the user asked to leave alone.

`FramesDropped` exists for the case where CSR cannot keep up: the capture callback
drops rather than queues, because queueing trades latency for throughput we do
not want. A non-zero count is the signal to lower the output resolution or turn
CSR off, and it is surfaced rather than hidden.

## Picker versus direct window targeting

Both are implemented, for different jobs:

- `StartWithPickerAsync` — `GraphicsCapturePicker`. The system UI *is* the consent
  mechanism, so this is correct the first time a user chooses a target.
- `StartForWindow` — `IGraphicsCaptureItemInterop.CreateForWindow`, no UI. This is
  what a saved per-game profile needs; a picker prompt on every launch would make
  profiles pointless.

## Status

| component | state |
|---|---|
| `Csr.Core` CSR maths | **verified** — 24/24 tests against the Python reference |
| `Csr.Capture` WGC + D3D11 | **compiles** (Windows-targeted, built on Linux CI) |
| HLSL `cs_5_0` shaders | written, **not yet compiled or run** |
| End-to-end on real hardware | **not done** — needs a Windows machine |

The shaders and the capture path have never executed. They are written against
the documented APIs and they compile, but "compiles" is not "works". The first
hardware run should check, in order: the golden-vector diff between the HLSL and
the CPU port, then `LastGpuMilliseconds`, then measured end-to-end latency.
