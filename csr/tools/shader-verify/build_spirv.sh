#!/usr/bin/env bash
# Compile the shipping HLSL to SPIR-V for CPU verification.
#
# The shipping target is cs_5_0 for D3D11, which only fxc/d3dcompiler can
# produce and only on Windows. This builds a Vulkan variant of the SAME HLSL
# source so the maths can be executed and diffed against the golden vectors on
# any machine.
#
# Binding shift is required and is a verification-only concern: D3D11 gives t#,
# u# and b# separate register spaces, so t0/u0/b0 legitimately coexist. Vulkan
# has one binding space per set, so without shifting all three collide on
# binding 0. The shipping shader is not modified.
#
#   t0 -> binding 0   (source texture)
#   b0 -> binding 1   (constants)
#   u0 -> binding 2   (destination image)
set -euo pipefail

SRC_DIR="$(cd "$(dirname "$0")/../../src/Csr.Capture/Shaders" && pwd)"
OUT_DIR="${1:-$(dirname "$0")/spv}"
mkdir -p "$OUT_DIR"

for name in CsrResolve CsrSharpen; do
    glslangValidator \
        -D -e main -S comp --target-env vulkan1.0 \
        --shift-texture-binding 0 \
        --shift-UBO-binding 1 \
        --shift-image-binding 2 \
        -o "$OUT_DIR/$name.spv" \
        "$SRC_DIR/$name.hlsl"
    spirv-val "$OUT_DIR/$name.spv"
    echo "  $name.spv  $(stat -c%s "$OUT_DIR/$name.spv") bytes  valid"
done
