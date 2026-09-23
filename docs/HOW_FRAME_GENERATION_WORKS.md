# How frame generation works

Frame generation (FG) inserts **synthetic frames** between rendered frames to
raise the displayed frame rate. FSR 3 Frame Generation and XeSS Frame Generation
both work roughly the same way:

1. The engine renders frame N and frame N+1 normally.
2. The FG runtime uses **motion vectors** and **optical flow** to estimate where
   objects moved between them.
3. It synthesizes an intermediate frame N+0.5 and displays it before N+1.

## Why it must be integrated by the game

Frame generation needs three things an external tool cannot provide:

- **The swapchain.** FG replaces the game's present path with a proxy swapchain
  so it can pace and insert frames. Hooking another process's swapchain reliably
  and safely is not something a configuration tool can do without fragile,
  anti-cheat-tripping injection.
- **Motion vectors.** Same engine data temporal upscaling needs.
- **UI composition.** The HUD must be excluded from motion estimation, or it
  smears. Only the engine knows which draw calls are UI.

Because of this, **there is no reliable, general way to add frame generation to a
game that did not ship it.** Any tool claiming to do so universally is either
overpromising or relying on brittle per-game hacks.

## Latency

FG increases *displayed* FPS but adds a small amount of latency, because a real
frame is held back to interpolate against. Vendors pair FG with a latency-reduction
path (e.g. anti-lag) to compensate. Frame generation is most useful when the base
frame rate is already reasonable (≈60 FPS+); at low base rates it can feel worse.

## What Universal FrameFX does

It reports one of four honest states per game/technology:

- **Disabled** — turned off in the profile.
- **Supported** — the game integrates this FG technology and your hardware/API
  can run it, so UFX can toggle it in the game's config.
- **Requires game integration** — the game does not ship this FG technology;
  UFX will not pretend otherwise.
- **Requires compatible implementation** — e.g. an FG that needs DX12 while the
  profile targets DX11.

It never fabricates frame-generation support or FPS gains.
