#!/usr/bin/env bash
# Build and test the platform-agnostic core on Linux/macOS (CI cross-check).
# The Win32 UI and D3D/Vulkan probes are Windows-only and are excluded here.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

echo "==> Configuring (core + tests only)"
cmake -S "$ROOT" -B "$ROOT/build" -DUFX_BUILD_APP=OFF -DUFX_BUILD_TESTS=ON

echo "==> Building"
cmake --build "$ROOT/build" --parallel

echo "==> Testing"
ctest --test-dir "$ROOT/build" --output-on-failure

echo "Done."
