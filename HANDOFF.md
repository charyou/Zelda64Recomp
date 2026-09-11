# Handoff

> 2026-09-11 — Run 3 complete: production scene-wide eligible AO/contact plus first Environment/Skylight Fill, independently default-off. Final build and focused Vulkan tests passed. User owns broader visual tuning and final Graphics-menu visual QA.

## Current result

The first narrow radius18 / 25%-ambient AO was checkpointed, then the user reported that F1 A/B was barely visible. The expanded implementation supersedes it:12 shared finite nearest-hit rays, radius45 contact, stronger configurable response, radius300 environment enclosure, and a bounded authored-fill response extending beyond per-pixel-lit characters to eligible opaque scene geometry. Environment Fill defaults to strength0.4 after the0.65 trial was too strong on the shield.

Read docs/RT_LIGHTING_VISION.md with docs/Addendum to RT_LIGHTING_VISION.md for accepted artistic/MM context; no repeated broad MM research was performed. **docs/RAYTRACING_FOUNDATION.md -> Run 3 — Ambient / Spatial Lighting** is the complete current technical delta. ADR-009 in docs/DECISIONS.md records the durable response/ownership choice; PER_PIXEL_LIGHTING.md records ambient-only RSP eligibility.

## Files, ownership and flow

- RT64 PrimaryHitRT.hlsl: same submitted opaque BLAS/TLAS and SurfaceHit; traceSurface returns nearest finite geometry/primitive/barycentric/distance data. One deterministic cosine sample set returns independent contact and longer-range enclosure visibility. Existing binary sun visibility stays separate.
- RaytracingDebug owns new RGBA16F spatial output (contact, environment visibility, oct-normalXY) beside the existing RGBA32F sun/call/clipW/primitive result. No duplicate AS, history/denoising or vendor-specific interfaces. TraceParams128 bytes, FramebufferParams80 bytes, both statically checked; RDPParams336 and raster varyings unchanged.
- SpatialLighting.hlsli / RasterPS / PerPixelLighting: exact RSP ambient is refined before unchanged direct-light addition. Conventional opaque texture-times-SHADE fallback instead reserves min(SHADE*budget, resolvedEnvironmentAmbient). This is an explicitly budgeted artistic approximation, not recovered original ambient or PBR. Unsupported/special/alpha content and Native retain original rendering. Original blobs remain. One shared floor bounds combined ambient darkening.
- Small MM bridge: patches/play_patches.c -> internal RecompEnvironmentFog31-word packet -> recomp_api.cpp / EnvironmentFog -> AtmosphereParameters::environmentFill / existing Workload snapshot. Publishes post-adjustment LightContext ambient RGB and a natural-sky/ordinary-room hemisphere-lobe hint. RT64 sees generic RGB/weight, no MM IDs. A finite TLAS miss is never interpreted as open sky. Skyless rooms use isotropic authored fill; missing culled geometry can still under-occlude it.
- WorkloadQueue atomics and DrawParams carry the controls/tuning on the existing config path. Parent config.hpp addition is in lib/N64ModernRuntime; JSON/reset/apply/bindings are in src/game/config.cpp, src/main/rt64_render_context.cpp, src/ui/ui_config.cpp. graphics.rml has Luna's narrowly scoped toggle/range/help/navigation integration; source/build/isolated-runtime copies match (final manual RML sync; see final-assets-synchronized.txt).

## Controls

F1 -> Game editor -> Lighting:
- RT AO / Contact Grounding — rt_ao, default false/Off.
- RT Environment / Skylight Fill — rt_environment_fill, default false/Off.
- Existing Experimental RT sun shadows remains independent.

Shared numeric JSON/Rml fields (also F1 sliders): rt_ao_radius45; rt_ao_strength0.8; rt_environment_radius300; rt_environment_strength0.4; rt_spatial_samples12; rt_spatial_bias0.1; rt_authored_fill_budget0.45; rt_ambient_floor0.35. See foundation table for bounds. F1 controls are session-local; Graphics Apply publishes persistent values. No quality-preset system was added.

Launch overrides: RT64_RT_AO=0|1, RT64_RT_ENVIRONMENT_FILL=0|1, existing RT64_RT_SHADOWS. ZELDA64RECOMP_DEV_INSPECTOR=1 opens the actual F1 inspector at setup when injected function keys fail (developer_mode required). Root used real mouse checkboxes; do not confuse the launch hook with an alternate fake toggle.

## Build and runtime evidence

Build command: pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1.

Final candidate: _working-directory/build-zelda-validation/Zelda64Recompiled.exe, SHA256 **963ACDEF7EF018A1C429B2FC1461DA4F0AAEBC9AE978AB42B08E6013DF7A0E45**. Same candidate at the isolated coverage/runtime/rt-ambient.exe and preserved in _working-directory/diagnostics/2026-09-10-rt-ambient/finished-run3/. The normal build assets and isolated runtime graphics.rml are synchronized. run3-cluster-build.log and run3-final-cluster-build.log record real shader objects/wrappers and successful Clang/LLD links, not dependency-only generation. DXIL compiled; D3D12 runtime was not tested.

Root directly tested Vulkan RX9070XT, copied mod stack, Enhanced per-pixel lighting, Atmospheric fog, original cutout-AA:
- Initial checkpoint: indoor AO-on and nighttime Town F1 on/off, sun shadows off.
- Expanded cluster: existing Debugger -> Pause (F4) held a Town workload for actual independent F1 off/AO/fill/both captures, with sun shadows enabled. cluster-paused-*.png and paused-pixel-check.txt are decisive evidence. AO post-contact mean RGB delta(-5.019,-3.109,-1.945); open floor(-0.538,-0.312,-0.274); sky unchanged. Localized AO is now measurable and visible without broad open-floor darkness.
- Fill A/B changed exposed flooring and enclosed character surfaces independently. These captures use trial fill strength0.65; final default0.4 is deliberately milder. Final rebuilt candidate passed title/attract Town and nighttime moon/Skull Kid smoke with AO/fill enabled and sun shadows disabled; final-attract-town.png and final-attract-night.png. No severe corruption/device loss or obvious fog/UI regression observed. Expanded skyless-interior A/B remains unqualified; do not claim it from the initial narrow indoor smoke.
- No broad MSAA/HFR/scene/mod matrix or performance benchmark. Paused workload removes scene/time motion but post-blend noise and UI animation can still differ.

Evidence/launcher: _working-directory/diagnostics/2026-09-10-rt-ambient/run.ps1 [-AO] [-Fill] [-Shadows] [-Town] [-Inspector]. Without -Town, never Start Game; attract scenes cycle automatically. With -Town, old finite input playback uses whichever copied save state is current. Do not assume old save a/bastian contents from stale docs. Launch/stop requires the documented interactive execution boundary.

## Preserved state and limitations

All test processes are stopped. The disposable saves from before the seed-based coexistence test were backed up under profile-backup/ and restored; save hashes are in final-restore-and-assets.txt; final RML synchronization hashes are in final-assets-synchronized.txt. Original user profile was not used for game writes. Existing rt-shadows.exe/finished-run2 evidence remains intact.

At run start parent HEAD a39610f and RT64 HEAD2f73d68 already contained Run-2 code, contrary to stale handoff claims. Run3 changes remain uncommitted in parent, RT64 and N64ModernRuntime. Preserve existing modified lighting Vision, supplied untracked addendum, docs/input-research, lib/rt64.7z and all other user work. No submodule reset/clean/update/replacement; Plume unchanged this run. Preserve Native/RDRAM, ADR-006 withdrawn camera publication and shader compile/dependency fixes.

Known sunrise shadow transient and separate camera/geometry shadow-collapse remain undiagnosed and were not investigated or masked. RT only sees submitted eligible geometry and one validated world projection. Unsupported cutouts/blends remain excluded. Sparse fixed samples can show directional bands/creases or motion-dependent changes. Authored-fill partition can redistribute baked lighting on conventional opaque textured surfaces; it is deliberately bounded but not a proof of physical material semantics. Existing blobs may overlap AO. No temporal IDs/motion vectors/history validation are supplied yet.

## Strongest continuation

Start with this final binary and the shared F1 radius/strength/budget controls; compare grounded posts/feet, skyless room palettes and camera motion against feature-off on the user's mod stack. Use the existing renderer pause for stable A/B. If a concrete artifact appears, improve the current shared signal/Surface Response path; preserve contact/environment separation and the backend-neutral raw hit/normal interface. Do not begin new MM research, vendor-specific denoising, local-light replacement, GI or a new RT scene merely to continue. The complete production cluster is present; next work should address evidenced coverage/quality limitations rather than reimplement its plumbing.
