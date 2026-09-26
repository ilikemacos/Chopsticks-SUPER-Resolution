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

Because of this, **there is no reliable, general way to add this *engine-integrated*
frame generation (FSR 3 FG / XeSS FG / DLSS 3) to a game that did not ship it.**
A tool claiming to add *that* universally is overpromising or relying on brittle
per-game injection.

## The one external kind that does work: optical-flow interpolation

There is a second, weaker kind of frame generation that **does** work on any app,
and it is worth being precise about rather than lumping it in with the above.
Instead of using engine motion vectors, it takes two frames the app already
presented, **estimates optical flow from the images themselves**, and synthesizes
a frame halfway between them (this is what Lossless Scaling ships). Our own
measured reference and quality numbers for it are in
[`../csr/FRAMEGEN.md`](../csr/FRAMEGEN.md): estimated-motion interpolation scores
+6 dB over motion-blind blending and lands within ~0.3 dB of a perfect-motion
ceiling, because interpolation is *bounded between two real frames* (unlike
temporal upscaling, which is a net loss externally — [`../csr/TEMPORAL.md`](../csr/TEMPORAL.md)).

Its honest costs are real and non-negotiable to state:

- **It adds latency and cannot remove it.** A real frame is held back to
  interpolate against, and there is no Reflex/anti-lag lever on another process's
  game. Displayed FPS rises; **input responsiveness gets worse.**
- **It interpolates, it does not render.** The synthetic frame is a guess between
  two real ones — smoothness, not new game state.
- **It smears on disocclusion, fast motion and HUD/UI**, because it has no engine
  data to mask the UI or resolve newly revealed regions. It needs an already-
  decent base frame rate (~60 FPS+).

So the accurate statement is: *engine-integrated* FG cannot be added externally;
*optical-flow interpolation* FG can, at the cost of latency and artifacts, and
must never be sold as free performance.

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
