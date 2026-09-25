#!/usr/bin/env bash
# Compile the CSR HLSL and diff it against the golden vectors.
#
# Needs: glslang-tools, spirv-tools, libvulkan-dev, mesa-vulkan-drivers, cc.
# No GPU required — llvmpipe provides a software Vulkan device, so this runs in
# CI.
set -euo pipefail
cd "$(dirname "$0")"

./build_spirv.sh
cc -O2 -o verify verify.c -lvulkan -lm

cases=$(ls ../../ref/golden/*.json | grep -v index | sed 's|.*/||; s|\.json||')
exec ./verify spv ../../ref/golden $cases
