# Handoff

> Current state: 2026-09-10, Run 2 finished. Directional hardware visibility and real Enhanced hard shadows passed focused Vulkan checks. User manually verified Graphics/F1 controls. Final normal project build succeeded; no further runtime check requested.

## Working result

- Mandatory Goal 1 passed: an independent sunlight visibility inset shows yellow unoccluded walls and blue occluded geometry in Town/save `A`. The primary-hit inset remains available and now renders upright.
- Mandatory Goal 2 passed for the narrow opaque/per-pixel-lighting subset: full-resolution hardware visibility attenuates only RSP directional diffuse terms matching the published environment sun. Ambient, other lights, fog, textures, combiner and framebuffer color are not multiplied by a shadow factor.
- Current candidate: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`, SHA256 `D8DF2EF071240676433F8DD02D14BE70B4CD3DF9651A8D645D85D4121F685CD8`. Isolated copy: `_working-directory/diagnostics/2026-09-07-coverage/runtime/rt-shadows.exe`.
- Persistent control: **Settings → Graphics → RT Sun Shadows → On (Experimental) → Apply**, default OFF. F1 → Game editor → Lighting also has a session toggle. The user manually verified both and closed the runtime. Graphics list now scrolls to keep the new row clear of Apply. Launch controls: `RT64_RT_SHADOWS=1` production; `RT64_RT_VISIBILITY=1` visibility inset; `RT64_RT_PRIMARY_HIT=1` existing primary oracle. Production is independent of the inset. When both inset variables are set, visibility takes display precedence.
- Read `docs/RAYTRACING_FOUNDATION.md` for exact new resource, shader, geometry and receiver contracts. `docs/PER_PIXEL_LIGHTING.md` remains authoritative for original lighting eligibility.

## Source state and build

Run 1 had been committed before this run: parent HEAD `5700e54551e614b0a5d8061ef794c5acbb2cc8ba`, RT64 `3fcf72b8b37789fd444106f3122507c8d315983e`, nested Plume `f425d3e69a4bb28a3357b2312faf7e9cae4566fd`. Run 2 changes are uncommitted. No submodules were reset, cleaned, updated or replaced; Plume required no further edits. N64ModernRuntime has one additional uncommitted default-false GraphicsConfig field for persistence; preserve that submodule change too. Preserve all existing work and untracked research.

Use `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1`. Clang/LLD build passed. RT library now uses `lib_6_5` for `GeometryIndex()`; initial `lib_6_3` DXIL validation correctly rejected it. Actual DXIL/SPIR-V objects and C/header wrappers regenerated, including all raster consumers of the extended 48-byte FramebufferParams. RDPParams remains 336 bytes and raster linkage is unchanged. Preserve the existing compile-then-separate-dependency-pass correction; `-MD` alone does not compile objects.

## Focused evidence

Evidence directory: `_working-directory/diagnostics/2026-09-10-rt-shadows/`.

- `primary-town.png`, `visibility-town.png`, `shadows-town.png`, `off-town.png` are real PNGs captured directly by root with computer-use. Primary shows Town/Link intersections upright. Visibility shows both real occlusion and visibility. Production has no inset and visibly changes supported shaded surfaces; OFF restores the usual Enhanced view.
- Static screenshot pixel checks: left pillar (30,550) OFF RGB46/25/20 versus ON15/9/9; ground (900,900) OFF42/22/15 versus ON31/16/11; sunlit wall (1500,500) stays18/17/12. Images share the camera but are not animation/time-frame-matched; do not claim a deterministic image diff.
- Vulkan RX9070XT; real copied mod profile, per-pixel lighting, Atmospheric fog, cutout-AA explicitly original. No black frames, geometry wedges or device-loss errors observed in the corrected focused checks. DXIL/D3D12 code compiled, but no D3D12 runtime, MSAA, broad transitions or performance qualification.
- User observed the intermediate inset vertically flipped. Fixed the inverse screen mapping to account for BOTH RSPProcessCS and RasterVS Y negations; the same correction aligns the production mask. Corrected candidate supersedes SHA24ACD4... .
- `run.ps1 -Mode off|primary|visibility|shadows` restores the copied seed and uses the existing finite 972-read `load-town.json`. That playback works with an exclusive fresh launch. Temporary menu investigations are not necessary to repeat. Early hidden instances remained alive after sandbox Stop-Process access denial and made setup unreliable. Launcher now refuses concurrent `rt-shadows` instances. Stop and wait for process exit before restoring/copying; interactive launch and stopping require the documented execution boundary. Root owns direct runtime navigation/capture; do not delegate these quick checks.
- Root stopped test processes and restored the copied seed at the mandatory checkpoint. The user subsequently performed manual tests and reports closing the runtime; no post-manual save reset was performed. Original profile was never used for game writes. Seed hash remains `B12F0C6F5546C59DF8E9CD26970A81F8F7CD11803E9F7D4E2E13E6D03D8D1C9B`.

## Completion and remaining issues

- Final documented build command succeeded after all source/UI changes. Normal build output has synchronized assets; final executable copied to the existing usable isolated runtime and preserved with evidence in `_working-directory/diagnostics/2026-09-10-rt-shadows/finished-run2/`. `final-build.log` and `final-hashes.txt` record completion and matching hashes. Pre-final known-good binary and all six user screenshots were preserved before final build/documentation. No additional runtime validation, per user instruction.
- Known issue, document only: shortly after sunrise, sun shadows can appear, almost vanish and then stabilize seconds later. User reports deep research identifying incompletely synchronized MM CURRENT_TIME, skyboxTime, light-setting/RGB transitions and sun elevation. This is a plausible explanation, NOT a confirmed diagnosis; no debugging performed here.
- Separate known issue: moving the camera into/behind nearby geometry caused temporary shadow collapse followed by recovery in at least one user test. Treat as a distinct camera/view-related issue; no cause established.
- User observations: many interiors lack meaningful direct lighting; some open/open-roof areas do not behave like normal sunlight receivers. These are coverage/art-direction observations, not proof of a single cause. No broad coverage fixes attempted.
- Post-checkpoint clipping refinement excludes inward/asymmetric custom clip ratios from secondary consumers; ordinary symmetric outward guard bands remain supported. Final source includes this gate. The user manual setting/runtime verification followed it.
- The next document, `docs/RT_LIGHTING_VISION.md`, is a forward-looking synthesis, not authorization or evidence of implemented AO/fill/GI. No next renderer feature is implemented. Run 3 starts with `RaytracingDebug::surfaces`, `PrimaryHitRT.hlsl::SurfaceHit`, and `traceVisibility`: extend existing replay-local geometry/call/primitive/barycentric identity for Surface Classification and AO.

Preserve Native/RDRAM and raster fallbacks, original lighting semantics, Atmospheric fog, ADR-006's withdrawn camera publication, existing shader build fixes and parked cutout-AA state. No historical `RT_ENABLED` renderer, AO, soft shadows, temporal filtering, denoising, reflection or GI infrastructure was enabled.
