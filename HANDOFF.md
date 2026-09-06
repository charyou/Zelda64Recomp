# Handoff

> Current state: 2026-09-07. The next renderer upgrade is implemented; see the focused feature and runtime documents.

## Current deliverable

- Branch: `codex/rt64-modern-fog`. Changes remain uncommitted; preceding RT64/N64ModernRuntime changes were preserved.
- Executable: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
- SHA-256: `8C910FA1F98C139A94FFDDC78B0990D468291AF0607F75DDB32D0ADD20FF0C9A` (subsequent source edits are comments/documentation only).
- Ready-to-use isolated profile launcher: `_working-directory/diagnostics/2026-09-06-lighting/Launch-Lighting.cmd`. It selects the correct profile CWD and clears developer playback, Native, fog, lighting and diagnostic overrides. Press Start Game normally.
- **Enhanced per-pixel diffuse lighting is enabled in Zelda.** It uses existing RSP lights/normals, supports batched skeletal transforms, and preserves authored normal magnitude. Native and unsupported draws remain legacy. The visible gain is smoother model shading, modest at the normal gameplay camera distance rather than a wholesale relight.
- Atmospheric follow-ups: nearby active collision-water coverage contributes moisture; automatic skyless views receive conservative distance haze. Fog was not broadly revalidated or recalibrated in this run.
- The prior camera-publication geometry regression remains fixed. No camera precision commands were reintroduced; ADR-006 still applies.

## Lighting behavior and controls

Read `docs/PER_PIXEL_LIGHTING.md` and ADR-007.

- `ZELDA64RECOMP_LIGHTING=original` restores original vertex lighting at launch; absent/default enables per-pixel lighting. Fog selection is independent.
- F1 Game editor / Lighting has a session-local checkbox and eligible/fallback counts. Native automation could not reliably open F1, so recorded A/B tests use separate launches.
- `RT64_LIGHTING_DIAGNOSTICS=1` emits periodic eligibility/rejection counts; ordinary runs do not emit the periodic trace.
- Eligibility uses actual draw vertices/light sets, with no actor/scene/mod IDs. Different skeletal world matrices are supported when affine and approximately uniform-scale. Modified colors, raw geometry, flat/unlit draws, mixed light sets, incompatible transforms, degenerate/varying-magnitude normals, and actual positional microcode lights retain legacy lighting.
- Runtime normals commonly have magnitude about 120, not 127. The first strict unit-normal prototype excluded all lit draws. Final code accepts consistent authored magnitude, preserves its mean, and falls back when length variation exceeds byte-quantization tolerance. Never infer enhancement coverage from a clean screenshot alone.
- Shared smooth RGB carries transformed normals only for eligible enhanced draws; alpha, UVs, position and raster output varying layout remain intact. The VS adds SV_VertexID; RDPParams is 336 bytes. Current HFR world matrices are shared with RSP processing.

## Validation and evidence

Evidence directory: `_working-directory/diagnostics/2026-09-06-lighting/`.

- Complete project-local Clang/LLD build passed, including patch regeneration and actual DXIL/SPIR-V shader compilation. Existing compiler warnings remain.
- Clean baseline and final original/per-pixel A/B at the Southern Swamp owl with a front-facing Link. Completion-timed playback substantially aligns pose/fairy position; it is not exact deterministic state replay. Final comparison: `captures/magnitude-original-front.jpg` and `captures/magnitude-perpixel-front.jpg`.
- Final title view logged 66 eligible / 2 legacy draws. Gameplay logged roughly 122-123 eligible draws, with unlit/unsupported draws retained as legacy. No shader sorting warnings after the bounded light loop was unrolled.
- Tested copied installation contains 44 `.nrm` archives plus one `.rtz` texture archive, and 46 enabled configuration entries. Prior documentation's 47 archive count included two loose built-in source files. This is a lightweight real-stack smoke test, not coverage of the user's entire roughly 100-mod installation or all content.
- Explicit D3D12 selection was attempted, but RT64's existing AMD driver workaround forced actual Vulkan. DXIL builds pass; D3D12 runtime remains unqualified. Do not bypass the workaround merely to claim coverage.
- Final Atmospheric combined smoke and explicitly logged Native/RDRAM reference both rendered cleanly. All test processes are stopped; copied seed saves/configuration were restored and mods remain unchanged. `final-runtime-manifest.json` records hashes, logs and successful captures. No broad scene, weather, indoor, or individual-mod survey was performed.

## Important build correction discovered by the experiment

**Bundled DXC `-MD` writes dependencies only, without compiling a shader object.** The preceding dependency fix therefore logged shader generation while leaving old binaries intact. The first new-layout lighting candidate paired CPU RDP336 with stale shader RDP320 and rendered corrupted geometry even with lighting disabled.

RT64 CMake now compiles first, then invokes a separate `-M -MF` dependency pass. All shader objects were genuinely regenerated. The earlier claim that unchanged shader hashes validated dependency tracking was insufficient and is superseded by the new source-change probe.

- `validate-dxc-outputs.ps1` proves fresh DXIL/SPIR-V objects exist, nested includes enter the depfile, dependency generation preserves the object, and changing an included constant changes both binary hashes.
- Reflection verifies SV_VertexID and 336-byte RDP parameters in the actual compiled VS.
- Targeted no-op shader builds report no work.
- Dynamic light loops also exposed re-spirv's cyclic-graph limitation. Final code explicitly unrolls the seven possible directional lights; no generic optimizer rewrite was needed.

The prior generated patch-registration dependency fix remains in place. Base-ROM generation still uses the corrected runtime recompiler; patch generation uses CI-pinned `a13e5cff...`. Generated patch sources are shared: do not build different patch candidates concurrently.

## Atmosphere

Original remains the persistent fog default and Native reference. Faithful and Atmospheric retain their prior camera/signature safety gates and authored optical-depth design. The user has manually tested the atmospheric look extensively; do not turn the next run into another broad fog qualification exercise without new evidence.

Water influence uses actual active static/dynamic collision water boxes, room/disabled-owner rules, nearby surface area and altitude. It blends toward the existing 0.65 wet-air transmittance target without hardcoded Swamp constants or changing MM weather. Missing water semantics contribute zero; overlapping custom boxes can overestimate the bounded influence. The masked event appends `waterInfluence`, preserving older field offsets; see `docs/MODERN_FOG_MODDING.md`.

Automatic skyless views use 15% optical strength with a minimum 10,000-unit distance scale. Short rooms receive negligible additional haze; authored fog still wins by maximum optical depth. Explicit mod FORCE_OFF remains zero. Indoor art direction and water-only dry-weather strength were not separately calibrated at runtime.

## Runtime/build workflow

Read `docs/RUNTIME_VALIDATION.md`. Native `node_repl` + `@oai/sky` works; browser CUA alone has no native support. Launch the interactive game with explicit CWD and Normal window style on the interactive desktop. Hidden launches were not targetable. Rebind/activate the returned window before capture; occluding windows can otherwise contaminate captures. Keyboard injection is unreliable; opt-in finite controller playback is the reproducible fallback.

Use `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1`. Windows PowerShell 5 with terminating native-stderr handling stopped on a CMake deprecation warning; PowerShell 7 works. The recipe initializes VS, LLVM 19.1.3, local tools/SDL and Windows SDK `mt.exe`.

Historical good/bad oracles remain intact. `_working-directory/build-zelda-clang/Zelda64Recompiled.exe` is still the bad historical oracle, not the deliverable. Historical geometry evidence remains in `docs/GRAPHICS_REGRESSION_HANDOFF.md`. Disk shortage from the preceding run is no longer a blocker.

## Next useful experiment

For a larger visible change, prototype improved character grounding/shadows using available draw/ground semantics, while preserving legacy shadows for unsupported content. Per-pixel positional lights are a natural extension of the new lighting path, but should not reinterpret MM's positional attenuation or discard mod lights. Keep experiments bounded and judge them in-game; no renderer/engine migration is justified by this run.
