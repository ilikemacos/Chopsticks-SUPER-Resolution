# Why external injection has limitations

This is the most important document in the project. Read it before filing a
feature request asking Universal FrameFX to "add FSR/XeSS/FG to any game."

## The five categories

Universal FrameFX classifies every integration into exactly one of these:

### 1. Native game support
The game ships the technology and exposes it in its own settings. UFX can read
and write the game's config so you can pick modes the launcher hides. **Safe.**

### 2. Mod / plugin support
The game has a modding interface (or a community mod loader) through which an
upscaler can be added at the source level of the mod. UFX can help you place
files but the mod does the integration. **Depends on the mod's legality/quality.**

### 3. DLL replacement / injection support
Some games load a vendor DLL (e.g. `nvngx_dlss.dll`, `libxess.dll`,
`amd_fidelityfx_dx12.dll`). Where a **publicly redistributable wrapper** exists
that translates one vendor's API to another (for example, presenting a DLSS
interface to the game while calling FSR 3 underneath), UFX can drop that wrapper
into the game folder with a full backup. This only works because the game already
produces the motion vectors/depth the wrapped upscaler needs. **Opt-in, per game,
reversible, never for anti-cheat titles.**

### 4. Compatibility-layer functionality
Spatial-only effects (FSR 1-style sharpening/scaling) can run as a post-process
in a compatibility layer because they need only the final image. This is genuinely
generic but is **not** the same as temporal FSR 2/3 or XeSS. UFX labels it clearly.

### 5. Technologies that cannot be implemented externally
Temporal upscaling and frame generation for a game with no integration and no
public wrapper. UFX will say so and explain why, rather than shipping something
that looks like it works but produces artifacts or trips anti-cheat.

## The core technical reason

Temporal reconstruction and frame generation need per-frame engine data —
motion vectors, depth, jitter, UI masks — that simply does not exist outside the
rendering engine. You cannot reconstruct correct motion vectors from a finished
frame. An external process can grab the back buffer, but not the geometry motion
that produced it.

## Anti-cheat

Injecting code into a running game process is exactly what cheats do, so
anti-cheat systems (EasyAntiCheat, BattlEye, VAC, Vanguard) detect and ban it.
Universal FrameFX **refuses** to modify folders containing anti-cheat binaries
and never injects into a running process. File-based, backed-up config edits to
single-player games are the safe boundary this project stays inside.
