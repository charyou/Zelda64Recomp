# Handoff

> Current-state document for the next development session.
> Update this file at the end of substantial work.
> Prefer current facts over historical narrative; Git contains the history.

## Current state

- Working branch: `codex/rt64-modern-fog`.
- Checkpoint `0faf84a` pins the clean RT64/Plume upgrade baseline to RT64 `5473732a822a4423b5696e7cb18fecc425a59875`.
- RT64 local branch `codex/modern-fog` commit `c8ce62b` contains the validated RDNA4 raster-ABI fix and the upstream PR #265 D3D12-to-Vulkan compatibility fallback; it follows fog implementation commit `05394e9` on the qualified upstream baseline.
- Recursive Plume checkout is `d890ac899e505fb30040e037a4037cdeca68f033`.
- N64ModernRuntime local branch `codex/os-set-time-reimplementation` commit `222fbfd` points to N64Recomp `dfd4a2d`, which adds the already-supported `osSetTime` runtime translation to the reimplemented-function list.
- Original is the default fog mode. Set `ZELDA64RECOMP_FOG_MODE` to `faithful` or `atmospheric` before launch, or press F5 to cycle Original → Faithful Per-Pixel → Atmospheric at runtime. F5 changes are printed to stdout.

## What changed

- Migrated Zelda's direct RT64 render-interface usage to Plume and updated Extended GBI MatrixGroup calls with behavior-neutral aspect/texcoord/LookAt semantics.
- Retained current RT64 clipping/framebuffer/rect improvements and Zelda's existing resolution-scaled texture LOD configuration.
- Added RT64 Workload fog mode and atmosphere metadata.
- Added high-resolution-only raster fog gating using the existing Native-vs-enhanced `FramebufferRenderer` split.
- Faithful mode reconstructs `tfPos.z/tfPos.w` from fragment `SV_Position.z` by inverting the draw's RSP viewport transform, then applies the draw-local `mul`/`offset` and existing N64 blender/fog color.
- Mixed per-vertex fog-state draws retain Original vertex fog and emit a one-time diagnostic.
- Added a recompiled-game ABI bridge that publishes resolved `PlayState.lightCtx` fog RGB, fogNear, and zFar before `Play_Draw`; `RT64Context::send_dl` latches them onto the current Workload.
- Atmospheric mode classifies baseline environment draws by exact RSP fog signature plus fog color. Matching draws use an inline exponential distance extinction constrained by legacy onset and original zFar; local overrides use Faithful mode.
- Atmospheric clip W is reconstructed from pixel-shader reciprocal `SV_Position.w`. Do not reintroduce the attempted `TEXCOORD1` fog-depth varying: it caused severe Vulkan corruption on RDNA4 and a D3D12 driver crash.
- Added RT64's pending RDNA4 compatibility detection from upstream PR #265: RX 90xx uses Vulkan automatically because current D3D12 drivers are known broken.
- Corrected the local N64Recomp symbol classification for `osSetTime`; without this, current N64ModernRuntime and the repository's older external CI recompiler both emit the game implementation and collide with the native runtime implementation at link time.

## Validation / build status

- Standalone RT64 `rt64` target builds successfully with MSVC/Ninja, including all modified DXIL and SPIR-V shader variants.
- Zelda's embedded `rt64/rt64.lib` target builds successfully (428 build steps in the configured scratch tree).
- A complete RelWithDebInfo Windows executable builds and links successfully at `_working-directory/build-zelda-clang/Zelda64Recompiled.exe` with Clang 19.1.3. The build generated 339 game C units, both RSP sources, 1,801 patch functions, all DXIL/SPIR-V shaders, and the final executable.
- The supplied compressed US ROM SHA-1 is `D6133ACE5AFAA0882CF214CF88DABA39E266C078`; the locally decompressed build input SHA-1 is `7F5630DBC4D5D61D6276213210C4D5CDD83A47D6`. Both ROMs and all generated sources remain ignored/uncommitted.
- Numerical viewport inversion check: 10,000 randomized viewport/fog samples reproduced the vertex-domain formula with maximum absolute error about `1.43e-14` in double precision.
- `git diff --check` passes apart from checkout line-ending notices.
- Runtime startup smoke tests passed for `original`, `faithful`, and `atmospheric`: each mode created a responsive `Zelda 64: Recompiled` window and remained alive through initialization.
- The first real in-game three-mode capture passed on Windows with an AMD Radeon RX 9070 XT using Vulkan. Original, Faithful Per-Pixel, and Atmospheric all rendered without color or geometry corruption. The opening-forest capture showed Original and Atmospheric substantially denser than Faithful, which is now a tuning/classification question rather than a stability failure.
- The initial shader implementation failed in-game: D3D12 crashed in `D3D12Core.dll` with `0xc0000005`; Vulkan produced alternating RGB-like corruption. HPFB Off and the Native/RDRAM view did not help. An A/B build that removed the new cross-stage varying rendered cleanly, proving the fault was the raster linkage change rather than the Plume migration, submodules, RDP buffer stride, or atmospheric parameter bridge.
- CPU layout assertions pass for the extended structs: `RDPParams` is 176 bytes with new fields at offsets 128/144/160; `FramebufferParams` is 24 bytes with `enhancedRenderer` at offset 20. Generated SPIR-V uses the matching 176-byte RDP array stride.

## Known issues and experimental behavior

- Faithful and Atmospheric fog passed the opening-forest in-game capture but still need validation at transition, transparency, water, particle, Lens of Truth, framebuffer-feedback, UI, and HFR hotspots.
- Real MM mixed-fog-index frequency is not yet known. Runtime instrumentation will report the first occurrence and safely fall back for that draw.
- Atmospheric fog is deliberately a first lightweight model. It has distance extinction and aerial-perspective color through the N64 blender, but no world-height term yet.
- The environment bridge can retain its last valid Play state briefly outside gameplay; exact signature classification makes accidental Atmospheric selection unlikely, but explicit game-state lifetime invalidation is a follow-up hardening task.
- The A/B control is an environment variable plus F5, not yet a persistent launcher graphics option.
- Atmospheric mode currently implements distance extinction/aerial-perspective color only. The planned subtle world-height density term was intentionally deferred until captures establish the distance model's baseline.

## Important discoveries

- `RasterPS` receives RSP viewport-transformed NDC depth. Inverting `viewport.scale.z` and `viewport.translate.z` recovers the same domain used by `RSPProcessCS` fog evaluation.
- `FramebufferRenderer(..., rtSupport=false, ...)` is the Native/RDRAM renderer; the Workload replay uses `rtSupport=true`. This is a clean shader-data gate without scale heuristics or shader permutation growth.
- `PlayState.lightCtx`, rather than `EnvironmentContext.lightSettings`, is the resolved state consumed by `Play_SetFog` after MM environment adjustments.
- `fogNear` is not interpreted as a world-space distance in Atmospheric mode. The legacy curve supplies authored onset while pre-quantization `zFar` supplies scene scale.
- On AMD Radeon RX 9070 XT, adding a fog-depth `TEXCOORD1` varying to RT64's linked raster shaders corrupts Vulkan output and coincides with a D3D12 driver crash. Pixel-shader `SV_Position.w` already supplies reciprocal clip W, so `1 / abs(SV_Position.w)` provides the distance input without changing the stage-link ABI.
- A non-recursive initial clone was not responsible for the runtime failure. All seven direct submodules and all 15 nested RT64 submodules were checked against their gitlink SHAs and matched exactly.
- The raw Windows build tree is intentionally not a self-contained package. `BUILDING.md` requires launching from the repository root or copying `assets/` beside the executable; release workflows separately bundle `assets/` and `recompcontrollerdb.txt`.
- The checked-in CI N64Recomp pin (`a13e5cff96686776b0e03baf23923e3c1927b770`) can compile this repository's patch ELF, while the newer runtime submodule tool can compile the base ROM. The old tool omits `osSetTime` from its reimplemented list; the newer tool crashes on the older patch configuration. The successful local build therefore used the exact CI tool for `patches.toml` and the corrected runtime-submodule tool for `us.rev1.toml`.
- On Windows, configure the application with Clang/clang-cl. MSVC configuration reaches compilation but fails because upstream ultramodern passes Clang-style `-Wno-unused-parameter` to `cl.exe`.

## Failed approaches worth not repeating

- Do not build the root executable with configure-only placeholder recompilation/RSP sources; they can validate CMake configuration but not the game.
- Do not use the CI-pinned N64Recomp binary for the base game without the `osSetTime` correction; the final link gets duplicate `osSetTime` definitions.
- Do not use N64ModernRuntime's newer N64Recomp for this checkout's `patches.toml`; it fails in strict patch recompilation. Keep the two generator roles separate until their versions are unified.
- Do not derive a Float Projection from a clean perspective formula alone. It would bypass MM's post-projection distortion.
- Do not apply atmosphere as a fullscreen depth pass or replace draw-local fog with frame-global values; both break transparent/local effects.

## Relevant commits / checkpoints

- `0faf84a` — clean exact RT64/Plume integration baseline.
- `9b3d2a1` — Zelda environment bridge plus fog-mode integration and verified renderer/runtime gitlinks.
- RT64 `05394e9` — enhanced raster fog modes, based on upstream `5473732a...`.
- RT64 `c8ce62b` — preserve the raster linkage ABI, reconstruct clip W in the pixel shader, and apply the RDNA4 Vulkan fallback.
- N64Recomp `dfd4a2d` / N64ModernRuntime `222fbfd` — route `osSetTime` through the existing runtime translation.

## Next recommended step

Finish the capture matrix at transition, transparency/water/particle, UI, framebuffer, Lens of Truth, and HFR hotspots. Compare Faithful against Original closely in the opening forest, inspect mixed-fog warnings/environment classification, then tune distance extinction and add a subtle optional world-height density term. Promote the fog mode into the persistent graphics UI only after those semantics are visually qualified. After fog is qualified, implement distortion-preserving Float View/Projection metadata.
