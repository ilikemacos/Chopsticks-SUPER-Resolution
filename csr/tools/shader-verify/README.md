# CSR shader verification

Compiles the shipping HLSL and **executes** it, diffing the result against
`csr/ref/golden/*.json` — the same golden vectors the C# port is tested against.

```bash
apt-get install -y glslang-tools spirv-tools libvulkan-dev mesa-vulkan-drivers
./run.sh
```

No GPU needed: Mesa's llvmpipe provides a software Vulkan device, so this runs
anywhere including CI.

## Result

All 9 cases × 2 passes match, worst deviation **4.4e-6** against a tolerance of
1/255 (0.0039) — roughly 900× inside tolerance, i.e. float-precision agreement
between the HLSL and the Python reference.

## What this proves, and what it does not

**Proves:** the maths as written in `CsrResolve.hlsl` and `CsrSharpen.hlsl` is
correct, and agrees with both the Python specification and the C# port.

**Does not prove:**

1. **That `fxc` compiles the same source to `cs_5_0`.** The shipping target is
   D3D11 shader model 5.0, which only `fxc`/`d3dcompiler` can produce, on
   Windows. This harness compiles the same HLSL to SPIR-V via glslang instead.
   No SM6-only constructs are used, so it should translate — but "should" is not
   "does", and that check needs a Windows machine.
2. **That a real GPU lands inside tolerance.** llvmpipe computes `rcp`/`rsqrt`
   at full float precision. Real hardware uses fast approximations, which is
   exactly why the tolerance is 1/255 rather than exact. A GPU could deviate
   more than llvmpipe does.
3. **Anything about the capture path.** `Csr.Capture` is not exercised here.

## Why the bindings are shifted

`build_spirv.sh` passes `--shift-texture-binding 0 --shift-UBO-binding 1
--shift-image-binding 2`.

D3D11 gives `t#`, `u#` and `b#` separate register spaces, so `t0`, `u0` and `b0`
coexist legitimately — the shipping shader is correct as written. Vulkan has one
binding space per descriptor set, so without shifting all three collide on
binding 0 and the SPIR-V is unusable. This is a verification-only concern and the
shader source is not modified.

## Mutation testing

A verification harness that cannot fail is worthless, so this one was checked by
deliberately breaking the shader:

| mutation | result |
|---|---|
| Rec.709 luma weight `0.2126` → `0.2000` | 2 comparisons fail (0.034 deviation) |
| deringing clamp removed | 12 comparisons fail (up to 0.25 deviation) |

Note on the clamp: testing it against only `checker_2x` and
`edge_diagonal_1_5x` reports **no failure**, because on binary 0/1 content the
final `[0,1]` clip already bounds the overshoot the clamp exists to prevent. Only
midtone content, where overshoot stays inside range, exercises it. `noise`,
`gradient`, `colour_bands` and `soft_midtone` do catch it; the dedicated
`midtone_square_1_5x` and `midtone_diagonal_1_5x` cases were added to catch it
loudly and on purpose rather than incidentally.

The lesson generalises: mutation-test against the **whole** case set, or a
partial run will tell you a covered behaviour is uncovered.
