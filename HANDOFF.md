# Handoff — D3D12 final-image fix (Plume descriptor layout), D3D12 qualification, FSR Ray Regeneration backend, receiver combiner widening (ADR-018), 2026-09-26

## Current state

The work is uncommitted, on parent `44325f1`, RT64 `466e890` and Plume `259e254`.

**To commit, follow `_working-directory/diagnostics/2026-09-26-d3d12-perpixel/commit-plan/COMMIT_PLAN.md`.** It has verified per-commit patches:
- Plume P1–P4;
- RT64 R1–R5, where receiver widening is R5 and independently revertible;
- parent Z1–Z3.

It also covers what not to commit. The Plume upstream classification and recommendations are in `docs/upstream/PLUME_UPSTREAM_REVIEW.md`.
- **RT64 modified files:**
  - `CMakeLists.txt`;
  - `rt64_application.cpp`, `rt64_lighting_instrumentation.cpp`, `rt64_framebuffer_renderer.cpp`;
  - `rt64_indirect_reconstruction.{h,cpp}`, `rt64_raster_shader.cpp`, `rt64_raytracing_debug.{h,cpp}`;
  - `PostBlendDitherNoisePS.hlsl`, `shared/rt64_spatial_receiver.h`.
- **RT64 new files:** `rt64_indirect_reconstruction_ffx.{h,cpp}`, `IndirectRayRegenInputsCS.hlsl`, `IndirectRayRegenOutputCS.hlsl`, and `tests/spatial_receiver_fixture.cpp` (CMake target `rt64_spatial_receiver_fixture`).
- **Plume:** `plume_d3d12.cpp` (DXR root signatures, readback copy, logging/DRED; plus the descriptor-layout fix below) and `plume_d3d12.h`.
- **Parent:** `src/main/main.cpp` (developer crash trace and window-size override) plus documentation.
- **Build configuration:** `_working-directory/build-zelda-validation` is configured with `RT64_FFX_SDK_DIR=_working-directory/research/fsr-sdk-2.3.0/include/Kits/FidelityFX`. The headers were fetched from the tagged SDK repository. Without that option the backend is compiled out and reports "not built".

The current executable is `_working-directory/diagnostics/2026-09-26-d3d12-perpixel/cand-fix/Zelda64Recompiled.exe` (identical to `build-zelda-validation`), SHA256 `7EA5C05A274BA571E11DC04152C07A88A2F92A127608236AC5B7CDC214678B2F`, with the AMD DLLs beside it.

The pre-fix final `…/2026-09-25-d3d12-rr/final` (`E2458055…1F57`) and candidates `cand-a` … `cand-g` in that directory remain the exact binaries behind the earlier measurements. Their D3D12 final images are wrong (see below).

The AMD FSR SDK v2.3.0 prebuilt zip (127,451,548 bytes, SHA256 `f90890b9…422273`, downloaded with the user's approval) is in `_working-directory/research/fsr-sdk-2.3.0/`. The DLLs are in `bin/`. Nothing AMD-derived is committed.

## D3D12 final-image fix (ADR-018 fix 7, this session)

A read-only audit (`_working-directory/diagnostics/2026-09-26-visual-regression-readonly/REPORT.md`) showed the D3D12 Enhanced final raster at about 50% of Vulkan's energy. The earlier qualification had only compared intermediate RT+ signals.

**Cause:** a generic Plume D3D12 bug.
- `D3D12DescriptorSet` gave immutable samplers view-heap slots, which the root signature (static samplers) does not have.
- In RT64's common raster set (18 immutable samplers at bindings 7–24), every later view was therefore read 18 slots early.
- Per-pixel lighting's t37 lights and t68–t70 normals and world transforms read zero on D3D12.

**Fix:** immutable samplers take no heap slot, and `setSampler` ignores them. It is two small hunks in `plume_d3d12.{h,cpp}`. No RT64 or parent code changed.

**Evidence** (`_working-directory/diagnostics/2026-09-26-d3d12-perpixel/REPORT.md`):
- Diagnostic view 12 was black on D3D12 and yellow on Vulkan; the fix alone restores it.
- Same binary, 2134×1200 target, final composed raster, Vulkan vs D3D12 mean luminance:
  - noon, Atmospheric: 0.2532 / 0.2531;
  - noon, Original: 0.2466 / 0.2466;
  - 23:00: 0.1977 / 0.1981.
- All three were inspected at full resolution. Only animated actors and flicker differ.
- Pre-fix D3D12 with per-pixel lighting and no RT features rendered lit geometry black; this is fixed.
- The D3D12 menu is fine.

**Pre-session temporal oracle** (Vulkan, unchanged executable): native 640×360 tiles stitched to 1920×1080 at a 2134×1200 target, plus window captures.
- Mean luminance: 0.2592 before vs 0.2532 now.
- This is fully attributed to the receiver widening (`cand-f` → `cand-g` on Vulkan). The other steps are at the run-to-run floor.
- The change is local to walls, stall fabric and Link, which get slightly lower shadow contrast. Exposure, fog and sky are unchanged.

Upstream Plume `main` (`d723793`) still has the bug. It is a separate upstream candidate; nothing was proposed upstream.

## What was decided and built (ADR-018)

**Genuine D3D12 RT+ works on the RX 9070 XT** (driver 32.0.31041.1004).
- The RDNA4 gate was stale: the Sep 2026 crash was the ADR-004 raster-varying issue.
- Six generic fixes were needed, none hardware-specific:
  1. DXC-valid runtime-specialized raster entries (`float1` scalar varyings, `SV_Coverage` through a local);
  2. post-blend dither PS signature order;
  3. Plume DXR: global root signature, empty local root signature, identifier-only SBT;
  4. `UNORDERED_ACCESS` on RT+ UAV textures;
  5. Plume `copyTextureRegion` to buffers;
  6. PSO and `Close` failure logging, plus DRED.
- `Auto` still picks Vulkan on RDNA4. An explicit D3D12 choice is honored.
- D3D12 and Vulkan RT+ signals are equivalent within the harness run-to-run floor, and so is the final composed image after fix 7. Costs are equal.

**Ray Regeneration** is an optional temporal backend behind `IndirectReconstruction` (`RT64_RT_GI_RECONSTRUCTION=rayregen`, D3D12 only).
- The documented input mapping held, with two additions:
  - the material channel carries raw GI validity, which prevents inactive-pixel bleed;
  - the specular albedo must be zero-initialized.
- The +Z view convention is solved by mirroring the view.
- Stability bias defaults to 0.25; the vendor default of 1.0 lost 14% energy.
- It falls back to the project backend with a logged reason. Validation is clean.
- Details: `docs/INDIRECT_RECONSTRUCTION.md`.

**Receiver gate widened** (secondary audit).
- `spatialCombiner` was the only active rejector. It rejected 48% of visible RT-hit pixels (Clock Town walls, Link's tunic), which rendered nearly black in sun shadow.
- A SHADE-free cycle 0 is now accepted when it is followed by `COMBINED × SHADE`. Coverage rose to 100%.
- **Durable contract:** `docs/SPATIAL_LIGHTING.md` → "Receiver and ownership boundary", covering:
  - the principle and per-draw gate;
  - the accepted and rejected forms;
  - independence from source authority;
  - a game-agnostic rule with MM-only qualification.
- **Fixture:** `rt64_spatial_receiver_fixture` (17 cases, all pass). It fails exactly the two widened cases against the pre-widening predicate.
- The user's exploratory observations are recorded there as future qualification targets: cutscenes, shop sheets, night flicker, intro ground.
- Receiver records now include the decoded colour combiner.

**No additional Ray Regeneration signal was integrated.** Each candidate needs a new signal policy owned elsewhere (see INDIRECT_RECONSTRUCTION.md).

## Evidence

All numbers are D3D12 on the RX 9070 XT, Clock Town at noon, deterministic playback, product target 2134×1200. Scripts and runs are in `_working-directory/diagnostics/2026-09-25-d3d12-rr/`; run folders are under `2026-09-25-wp3/` (`hd-*`, `pb-*`, `rcv-*`, `fin-*`).

| | Project temporal | Ray Regeneration |
|---|---|---|
| Noise, still | 0.00088 | 0.00045 |
| Noise, turn | 0.00122 | 0.00036 |
| Instability, turn | 0.00090 | 0.00077 |
| Energy vs spatial | 0.997 | 0.98–0.99 |
| Reconstruction cost | 0.40 ms | 3.39 ms |
| Whole Workload | 3.40 ms | 6.49 ms |
| Memory | +82 MB | 321 MiB vendor (236 MiB aliasable) + about 100 MB adapter |

- Ray Regeneration softens contact detail in motion.
- Faceting is present in the raw signal and the geometric-normal guide for every backend. Better denoising only makes it relatively more visible.

## Known limits / open

- Ray Regeneration is not the product default at 8× the cost. Its disocclusion lag and moving-light behaviour were not measured beyond the camera turn.
- Faceting needs a GI-generation decision (a smooth hemisphere normal), not a reconstruction change.
- The actor indirect-shadow lag (ADR-017) is unchanged.
- The wider receiver gate is qualified only in Clock Town at noon.
  - Its "near-black walls" evidence was gathered on pre-fix D3D12.
  - On Vulkan, its effect is a modest local change: −2.5% mean luminance and flatter brick shadow.
  - Other scenes, night and mods are not yet A/B'd.
- D3D12 final-image equivalence is established only in Clock Town (day and night, both fog modes).
- Automatic API selection is not changed for RDNA4, and D3D12 HFR image metrics are unqualified.
- Env-gated developer hooks are classified in COMMIT_PLAN.md. All are kept, and none is disposable:
  - required by validation: `ZELDA64RECOMP_DEV_WINDOW_SIZE`, `RT64_LIGHTING_CAPTURE_SLICE_MIB`;
  - generic diagnostics: `ZELDA64RECOMP_CRASH_TRACE`, `PLUME_D3D12_DRED` plus HRESULT logging, the receiver-row `color_combiner`;
  - Ray Regeneration developer surface: `RT64_RT_RAYREGEN_CONFIG`, `RT64_RT_RAYREGEN_VALIDATION`, `RT64_FFX_LOADER_PATH`.
- Plume `rtDummyGlobalPipelineLayout` is dead after the DXR fix. It is left in place locally.

## Next meaningful decision

Commit per COMMIT_PLAN.md. Then qualify the receiver-gate widening across representative scenes (interiors and shops, night, cutscenes, the intro, Termina Field, a mod stack), because it changes the most pixels. First classify each of the recorded exploratory observations with the existing receiver instrumentation.

Then decide on GI-generation normals for low-poly faceting, for example gathering over the interpolated shading normal while the guide keeps the geometric normal. Which reconstruction backend is used matters less than either of these.

## Preserve

Keep the untracked `docs/AGENTS_RT64.md`, `docs/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md`, `docs/input-research/`, the older and red-team reviews, `start-rtplus-dev.bat` and `lib/rt64.7z`. ROMs, captures, SDK downloads and binaries stay in `_working-directory/`. `CHANGELOG.md` stays on its release workflow.
