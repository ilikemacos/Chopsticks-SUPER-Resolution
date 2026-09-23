# Game integration

This document explains, step by step, what Universal FrameFX does when you apply
a profile to a game — and what it deliberately will not do.

## The Apply / Backup / Restore / Remove workflow

Every game panel exposes four actions:

- **Apply** — write the profile's settings into the game.
- **Backup** — snapshot the files Apply would touch (Apply always does this
  first automatically; the button lets you snapshot on demand).
- **Restore** — revert the game's files to a chosen backup snapshot.
- **Remove Integration** — restore the original files and delete UFX's changes.

## What Apply actually does

1. **Detect the game directory** from the profile's executable path.
2. **Anti-cheat check.** If EasyAntiCheat, BattlEye, VAC or a similar marker is
   found anywhere in the folder, Apply refuses and explains why. This protects
   you from bans.
3. **Backup.** The affected files (the game's config, and any DLL that a
   DLL-swap would replace) are copied into
   `%LOCALAPPDATA%\UniversalFrameFX\backups\<timestamp>\` with metadata.
4. **Apply the change**, one of:
   - *Config-only* (default): edit the game's own `.ini`/`.cfg`/`.json` keys.
   - *DLL-swap* (opt-in per profile): copy a publicly redistributable wrapper DLL
     into the game folder. The original is in the backup.
5. **Record** the integration so Remove can undo it precisely.

Nothing outside the game folder is ever modified. No Windows system file is
touched. No running process is injected into.

## Config-only vs DLL-swap

- **Config-only** is always safe and always reversible. Use it when the game
  already ships the upscaler and you just want a mode the launcher hides.
- **DLL-swap** is only offered when a legal, redistributable wrapper exists for
  that game's situation, and only after you opt in. It is still reversible via
  the backup, but you should never use it on online/anti-cheat titles.

## Obtaining vendor SDKs / runtimes

Universal FrameFX ships **no proprietary vendor binaries**. If a feature needs a
vendor runtime (for example, the XeSS runtime DLL), the app tells you exactly
which file is needed and links to the vendor's official, redistributable source.
You obtain it legally; UFX only places the file you provide, with a backup.

## Xbox / Microsoft Store games

These install under `WindowsApps` with restrictive ACLs and are code-signed. UFX
can detect them but generally cannot (and should not) modify them. It will say so
rather than fail silently.
