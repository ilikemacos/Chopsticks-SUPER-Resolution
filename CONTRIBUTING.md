# Contributing to Universal FrameFX

Thanks for your interest! This project has one non-negotiable rule above all
others:

> **Never fake support for a graphics technology.** If something cannot be done
> reliably and legally, we say so. Technical correctness beats impressive-sounding
> claims, every time.

## Ground rules

- **No proprietary vendor binaries** in the repository. If a feature needs a
  vendor SDK/runtime, design it so the user obtains it legally and document how.
- **No injection into running processes**, no anti-cheat evasion, no modifying
  Windows system files.
- Every game-file modification must be **backed up and reversible**.
- Claims in the UI, README, docs and website must be **verifiable**. If you add a
  compatibility-table cell, cite the vendor documentation in the PR.

## Development setup

### Application (C++)

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

The platform-agnostic core and the full test suite also build on Linux/macOS
(used by CI as a cross-check); the Win32 UI and D3D/Vulkan probes are Windows-only.

### Website

```bash
cd website
npm install
npm run dev      # http://localhost:3000
npm run build
npm run lint
```

## Code style

- C++20, `/W4` clean (treat warnings as bugs).
- Prefer the existing `Result<T>` type over exceptions across module boundaries.
- Keep GPU/vendor logic honest: use capability probes, never brand assumptions,
  to decide availability. Brand may inform *defaults*, never *hide* options.

## Adding an upscaler or frame generator

1. Implement `IUpscaler` / `IFrameGenerator`.
2. Register it in `UpscalerRegistry` / `FrameGenRegistry`.
3. Fill in `Caps()` truthfully, including `requirements` and (for FG)
   `externalLimitation`.
4. Add unit tests, including the "unavailable with a clear reason" cases.
5. Update the compatibility table in `website/lib/compat.ts` and `docs/`.

## Pull requests

- Keep PRs focused. Include tests. Describe the *why*.
- CI must pass: Windows build, tests, static analysis, and the website build.
- By contributing you agree your work is licensed under the project's MIT license.

## Reporting issues

Include your **exported diagnostics report** (Diagnostics → Export) and the log
file. See `SECURITY.md` for vulnerability reports (do not open a public issue for
those).
