# Handoff

> Current state: 2026-09-09. Focused lighting-coverage pass completed after restart; no further shadow/cutout investigation was performed.

## Current deliverable

- Branch: `codex/rt64-modern-fog`. This continuation's changes are uncommitted; prior submodule changes and the user's AGENTS edit are preserved.
- Executable: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
- SHA-256: `7C3637B5034E3D30C6D6D06DEF66850AD288070663562949E1AA200A2D65E59A`.
- Isolated interactive launcher: `_working-directory/diagnostics/2026-09-07-coverage/Launch-Lighting.cmd`. Press Start Game normally. This selects the updated executable/profile, clears playback/tints and disables the parked cutout-AA experiment.
- Expanded per-pixel coverage preserves authored normal magnitude per vertex, including zero/short normals. Large Town wall/ground sections no longer fall back because of another vertex's magnitude.
- Equivalent light values can use different buffer indices. Shared-matrix draws evaluate the original local directional-light equation, supporting nonuniform scale/shear. Mixed transforms still need a compatible common basis.
- Fog and camera-publication behavior were not changed in this continuation.

## Lighting behavior and remaining boundaries

Read `docs/PER_PIXEL_LIGHTING.md` and ADR-007. F1 Lighting and `ZELDA64RECOMP_LIGHTING=original` remain the normal A/B controls. Native always uses original shade RGB.

- The dominant visible rejection in Town was the old draw-wide normal-length gate, including very short/zero normals. A temporary gate-relaxation experiment exposed this; correct per-vertex magnitude transport replaced the gate rather than forcing a draw-wide average.
- Spatial diagnostics showed Clock Tower interior walls/floor blue while characters were green. These surfaces lack a supported RSP light set and retain authored shading. Do not invent normals or reinterpret RGB to force them into diffuse lighting.
- Genuine mixed light/color state and unsupported mixed transforms still fall back per draw. Representative Town logs did not show mixed-state rejection dominating; no batch splitting was added.
- True positional lights still need pixel world position and transform scale to preserve the original anisotropic distance, diffuse clamp and wrapped attenuation. Existing directional-resolved MM point lights work. No new MM function patches or identity gates were added.
- `RT64_LIGHTING_COVERAGE=1` tints visible surfaces; `RT64_LIGHTING_DIAGNOSTICS=1` logs fallback categories. Temporary aggressive/conservative switches were removed.
- The scalar normal magnitude adds `TEXCOORD1`; dynamic/specialized SPIR-V and generated DXIL wrappers were rebuilt together. RDPParams remains 336 bytes; `pixelLighting.z` now selects a shared local matrix or the rotated basis.

## Validation and evidence

Evidence remains in `_working-directory/diagnostics/2026-09-07-coverage/` (created before the restart).

- Targeted project-local Clang/LLD build passed, with actual DXIL/SPIR-V generation. Final Vulkan runs had no shader-sorting/error messages in stderr.
- Town intro spatial captures: `captures/town-conservative-tint.jpg` shows extensive yellow normal rejection; `town-expanded-tint.jpg` shows the previously rejected environment enhanced. Early captures predate the shared-matrix refinement.
- Final untinted Town enhanced/original launches and a Native/RDRAM Town smoke check rendered coherently; captures are not frame-matched and do not establish pixel-exact equivalence. Legacy inn view also remained clean. F1 automation was unreliable, so launch-time lighting A/B was used.
- Representative final Town diagnostic: 263 eligible / 86 set fallbacks, no mixed/transform/positional rejection at that sampled view. This supports the spatial observation, not a global coverage percentage.
- Copied real profile: 44 `.nrm` and one `.rtz` archive. No mod-loading crash or obvious missing/corrupted geometry/materials observed. This is a stack smoke test, not exhaustive mod qualification.
- Actual backend Vulkan; DXIL compiled but D3D12 runtime remains unqualified. Cutout AA was explicitly disabled throughout this continuation.
- Test processes stopped and copied seed restored. Source user-profile save remains SHA-256 `B12F0C6F5546C59DF8E9CD26970A81F8F7CD11803E9F7D4E2E13E6D03D8D1C9B`.

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

## Parked side work

Before the restart, shadow inspection found that MM foot shadows already use light directions/floor collision. Projected mesh shadows need caster/receiver semantics and an overlap-safe mask/composite pass; no shadow implementation was delivered. Preserve that finding for a focused task.

The earlier cutout-MSAA prototype remains in the working tree, unvalidated and unchanged during this continuation. Details: `_working-directory/diagnostics/2026-09-07-cutout/experiment.md`. Its current source defaults on for eligible MSAA draws; the lighting launcher explicitly sets `RT64_CUTOUT_AA=original` to isolate it. Do not treat it as a qualified release feature. No further work on it is required for this lighting pass.
