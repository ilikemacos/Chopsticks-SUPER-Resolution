Universal FrameFX - Portable (PowerShell edition)
==================================================

This is a runnable, no-install, no-compiler edition of Universal FrameFX. It
uses Windows PowerShell + .NET Windows Forms, both built into Windows 11.

HOW TO RUN
----------
1. Extract this zip anywhere (e.g. your Desktop).
2. Double-click  UniversalFrameFX.cmd
   (or right-click UniversalFrameFX.ps1 -> Run with PowerShell)

If Windows SmartScreen warns about an unrecognized app, that is expected for
unsigned open-source scripts. You can inspect UniversalFrameFX.ps1 in any text
editor first - it is plain, readable PowerShell.

WHAT IT DOES
------------
- Detects your GPU(s): name, vendor, architecture, VRAM, driver, and DirectX /
  Vulkan availability - read from Windows, not guessed from the brand.
- Shows which upscalers (FSR 1/2/3, FSR 4, XeSS) are actually available for your
  hardware, with the real reason when one is not.
- Labels frame generation honestly (it cannot be added to a game that did not
  ship it).
- Manages per-game profiles as JSON under %APPDATA%\UniversalFrameFX\profiles.
- Backs up a game's config files before any change and can restore them.
- Refuses to touch folders that contain anti-cheat software.

WHAT IT DOES NOT DO
-------------------
- It does not add FSR/XeSS or frame generation to games that were not built for
  them (temporal upscalers need engine motion vectors that no external tool can
  supply).
- It never fabricates FPS numbers. Resolution figures are configuration
  estimates and are labelled as such.

Data, logs and backups live under %APPDATA%\UniversalFrameFX.
License: MIT. Not affiliated with AMD, Intel or NVIDIA.
