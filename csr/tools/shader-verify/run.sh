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

# The authoritative case list is index.json's "cases" array. Globbing *.json
# instead would sweep in files with a different schema — bilinear_baseline.json
# has no "resolve"/"full" keys — and abort the verifier before any case runs.
cases=$(python3 -c "import json,sys; print(' '.join(c['name'] for c in json.load(open('../../ref/golden/index.json'))['cases']))")
exec ./verify spv ../../ref/golden $cases
