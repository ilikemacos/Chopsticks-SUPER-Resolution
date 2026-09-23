# Security Policy

## Supported versions

Universal FrameFX is pre-1.0. Security fixes are applied to the latest release
and the `main` branch.

## Reporting a vulnerability

**Please do not open a public issue for security vulnerabilities.**

Instead, use GitHub's private vulnerability reporting:
**Security → Report a vulnerability** on the repository, or email the maintainers
listed in the repository profile.

Include:

- A description of the issue and its impact.
- Steps to reproduce (a minimal profile/config if relevant).
- Affected version and your OS/GPU/driver.
- Any relevant log excerpts (`%LOCALAPPDATA%\UniversalFrameFX\logs`).

We aim to acknowledge reports within 5 business days and to ship a fix or
mitigation as quickly as is practical, coordinating disclosure with you.

## Scope and threat model

Universal FrameFX is a local desktop tool. The security-relevant surfaces are:

- **Game-file modification.** All writes are confined to a game's own folder,
  preceded by a backup, and reversible. Report any path that writes outside the
  intended game directory, or any missing backup.
- **DLL-swap.** Only publicly redistributable wrapper DLLs the user opts into.
  Report any code path that fetches or places a binary without user consent or
  without a backup.
- **Installer.** `install.ps1` downloads only from the official GitHub Releases
  API and verifies SHA-256. Report any download that skips verification, any
  unnecessary elevation, or any change to Windows security settings.
- **Anti-cheat safety.** UFX refuses to modify anti-cheat-protected folders.
  Report any bypass.
- **Privacy.** Game-library and diagnostics data stay local. Report any code
  path that transmits user data without an explicit opt-in.

## What is out of scope

- Bans resulting from using file modifications on online/anti-cheat games
  against the documented warnings.
- Issues in third-party vendor SDKs the user supplies; report those to the vendor.
