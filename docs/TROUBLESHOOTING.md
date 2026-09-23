# Troubleshooting

## The app shows "No GPU detected"

- Universal FrameFX enumerates GPUs through DXGI. If nothing appears, your
  graphics driver may be in a bad state — reinstall the latest driver.
- Remote desktop / headless sessions may only expose the Microsoft Basic Render
  Driver (WARP). Run locally on the machine with the GPU.

## An upscaler shows "Unavailable"

Read the line underneath the status — it states the exact failed requirement,
for example:

- *"FSR 4 requires RDNA 4 hardware. Detected: RDNA 3."*
- *"This GPU does not expose Direct3D 12."*

This is not a bug; it is the honest capability of your configuration.

## Frame generation says "Requires game integration"

The selected game does not ship that frame-generation technology. There is no
external way to add it (see `INJECTION_LIMITATIONS.md`). Choose a game that
integrates it, or use upscaling without FG.

## Apply is blocked with an anti-cheat warning

The game folder contains an anti-cheat component. Modifying it risks a ban, so
UFX refuses. This is intentional and cannot be overridden for online titles.

## Changes didn't take effect in-game

- Confirm the profile's executable path points at the real game binary (not a
  launcher shim).
- Some games cache settings; fully restart the game.
- Verify the game actually supports the chosen upscaler natively — config-only
  mode cannot add support that isn't there.

## I want to undo everything

Use **Restore** (pick the snapshot) or **Remove Integration** on the game panel.
Backups live under `%LOCALAPPDATA%\UniversalFrameFX\backups\`.

## Where are the logs?

`%LOCALAPPDATA%\UniversalFrameFX\logs\universalframefx.log`. Attach this file to
any bug report. Use **Diagnostics → Export report** for a redacted system report.

## The installer was blocked by SmartScreen

Universal FrameFX never disables SmartScreen. Unsigned open-source binaries can
trigger a warning; verify the SHA-256 checksum against the release page, then
choose "Run anyway" if you trust it, or build from source.
