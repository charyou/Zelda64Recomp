# Handoff

> Current-state document for the next development session.
> Update this file at the end of substantial work.
> Prefer current facts over historical narrative; Git contains the history.

## Current state

- Working branch: `codex/rt64-modern-fog`.
- Checkpoint `0faf84a` pins the clean RT64/Plume upgrade baseline to RT64 `5473732a822a4423b5696e7cb18fecc425a59875`.
- RT64 local branch `codex/modern-fog` commit `05394e9` contains the generic enhanced-raster fog implementation and is based directly on that exact SHA.
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
- Corrected the local N64Recomp symbol classification for `osSetTime`; without this, current N64ModernRuntime and the repository's older external CI recompiler both emit the game implementation and collide with the native runtime implementation at link time.

## Validation / build status

- Standalone RT64 `rt64` target builds successfully with MSVC/Ninja, including all modified DXIL and SPIR-V shader variants.
- Zelda's embedded `rt64/rt64.lib` target builds successfully (428 build steps in the configured scratch tree).
- A complete RelWithDebInfo Windows executable builds and links successfully at `_working-directory/build-zelda-clang/Zelda64Recompiled.exe` with Clang 19.1.3. The build generated 339 game C units, both RSP sources, 1,801 patch functions, all DXIL/SPIR-V shaders, and the final executable.
- The supplied compressed US ROM SHA-1 is `D6133ACE5AFAA0882CF214CF88DABA39E266C078`; the locally decompressed build input SHA-1 is `7F5630DBC4D5D61D6276213210C4D5CDD83A47D6`. Both ROMs and all generated sources remain ignored/uncommitted.
- Numerical viewport inversion check: 10,000 randomized viewport/fog samples reproduced the vertex-domain formula with maximum absolute error about `1.43e-14` in double precision.
- `git diff --check` passes apart from checkout line-ending notices.
- Runtime startup smoke tests passed for `original`, `faithful`, and `atmospheric`: each mode created a responsive `Zelda 64: Recompiled` window and remained alive through initialization.
- In-game hotspot validation is still outstanding. The Windows automation helper could not target the SDL/Vulkan window, so it could not activate the focused `Start game` launcher action even though a valid stored ROM was detected.

## Known issues and experimental behavior

- Faithful and Atmospheric fog compile but still need in-game capture validation at the documented fog, transparency, water, particle, Lens of Truth, framebuffer-feedback, UI, and HFR hotspots.
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
- N64Recomp `dfd4a2d` / N64ModernRuntime `222fbfd` — route `osSetTime` through the existing runtime translation.

## Next recommended step

Run an in-game three-mode capture matrix at the report's fog, transition, transparency/water/particle, UI, framebuffer, Lens of Truth, and HFR hotspots. First inspect the mixed-fog warning and environment/local classification; then tune/add the world-height term and promote the fog mode into the persistent graphics UI. After fog is visually qualified, implement distortion-preserving Float View/Projection metadata.
