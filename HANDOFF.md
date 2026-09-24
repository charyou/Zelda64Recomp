# Handoff — RT scene coverage and primary visibility authority, 2026-09-24

## Current state

Uncommitted, commit-ready changes on top of parent `bff648c` / RT64 `ccb86d2` / Plume `91e6711`. The parent gitlink `lib/rt64` shows as modified; Plume and N64ModernRuntime are unchanged. The basis is `docs/reviews/RT_PLUS_ARCHITECTURE_REASSESSMENT_2026-09-24.md`; new durable decisions are ADR-014 and ADR-015 (plus an ADR-004 amendment).

Implemented:
- **Primary receiver depth coverage (F1).** `PrimaryRayGen` spans N64 GL-style NDC z −1..1. The old D3D-style 0..0.99 window dropped receivers nearer than about 20 units and farther than about 1.3k–1.7k units.
- **Per-backend receiver clip W (F2).** RasterPS uses `1/SV_Position.w` only under `__spirv__` and `SV_Position.w` on DXIL. The SPIR-V path is byte-identical in behaviour.
- **FullSync metadata.** A later FullSync inside the same task now carries `fogMode`/`perPixelLighting`/`atmosphere` into the next Workload (`rt64_state.cpp`); the next task still overwrites them.
- **Environment directional visibility authority (ADR-014).**
  - The adapter publishes `environmentDirection[i].w` = `smoothstep(0, 0.1, normalized y)`, from MM's `ActorShadow_DrawFeet` `dir.y > 0` rule (`src/main/rt64_render_context.cpp`).
  - In RT64, authority scales owned-term shadowing (`lerp(1, traced, a)`), ADR-012 gain (`a·DirectAuthority`), ADR-013 additions (`traced·a`; no visibility means no addition) and GI primary transport. Authority 0 traces no primary rays.
  - Expansion (bit 64) now requests its own visibility rays, so additions stay occluded with raster sun shadows off.
  - `FramebufferParams.shadowSun` became `primaryVisibility` (traced, authority, reserved, owned-application); all layout sizes are unchanged.
- **Room occluder completion (ADR-015).** The MM `Room_Draw` patch additionally submits opaque cullable-room entries whose bounding sphere is entirely behind the camera (the exact complement of MM's test). They produce zero raster pixels; XLU order and the beyond-zFar cull are unchanged. Host import `recomp_room_occluder_completion_enabled` (0x8F0000EC); `ZELDA64RECOMP_ROOM_OCCLUDERS=0` gives A/B.
- **Scene series (developer).** `RT64_RT_SCENE_SERIES=<file>` writes per-occurrence JSONL of CPU collection facts, stock transform-group identity triangles and production raygen counters. They are written only when `environment[5].x` is set (formerly a dead duplicate); there is no policy input.
- **API logging.** Startup stderr logs the requested and chosen graphics API.

## Build

Final full build (patches ELF, N64Recomp patches, all RT/raster SPIR-V/DXIL consumers, executable) passed. Executable SHA256 `4774970C324DD3C9F6249009F1D8276C979294E314831FD491DCAA3344E68128` at `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`. Build script: `_working-directory/diagnostics/2026-09-24-scene/build.ps1`.

## Runtime evidence (Vulkan RX 9070 XT, `_working-directory/diagnostics/2026-09-24-scene/`)

**Camera motion**, deterministic `motion-arc.json` in noon Town, 20 Hz and HFR:
- No whole-frame RT disable. World-camera agreement never rejected a range. HFR occurrences of one game frame carry identical scenes.
- The old window lost 8–30% of primary hits near the camera in motion and up to 43% of the frame against walls (`motion13-old-vs-new.png`, view 13). The far loss was 1.4% of hits in Termina Field.
- Remaining triangle churn is game culling, dominated by tagged actors. Room geometry is stable. Room occluder A/B added up to 427 triangles and about 860 blocked pixels per frame.

**Night 23:00:**
- The primary points down (y −0.98) with RGB (0.39, 0.51, 0.24). The old build traced these rays into the ground; visibility gating alone over-brightened undersides via the 1.77× gain.
- The final build traces 0 rays and matches the authored magnitude (`night-link-4way.png`).
- The arch shading on the wall is authored (present with master off).

**Noon:** authority 1, all hits traced. With raster shadows off, visibility is still traced for expansion.

**Benchmark** (clean 16-sample Town noon, old/new): whole 0.851/0.863 ms, AS build 0.397/0.398 ms, fused RT 0.161/0.150 ms, no resource growth (`performance.json`). Neutral.

## Limits / open

- **D3D12 runtime is unqualified.** RT64 forces Vulkan on RDNA4 drivers ≤ Aug 2026 even when D3D12 is requested; the DXIL clip-W convention is verified only at DXC level. The earlier view-13 "D3D12" captures were actually Vulkan.
- Off-screen culled actors do not cast (actor draw culling is gameplay-coupled; no persistence, per ADR-015).
- The secondary directional is published with authority but has no RT visibility realization.
- A full BLAS/TLAS rebuild still happens every framebuffer × occurrence (AS build is about 46% of the workload; about 7× per game frame at HFR).
- Temporal identity, motion, reconstruction seam and the semantic packet transport (reassessment F3/F5/F8/F9/F10) are unchanged.
- No broad scene/mod/MSAA qualification. Physical F9 delivery remains unqualified as before.

## Next

The scene lifetime package:
- a per-Workload static/dynamic partition keyed by stock transform groups;
- reuse or refit across HFR occurrences (Plume has no AS update API yet; a static/dynamic BLAS split needs none);
- a separate RT+ requirement flag instead of the repurposed stock `raytracingEnabled`.

Then source collection with stable IDs (F3), reusing ADR-014's per-source authority pattern.

Preserve the supplied untracked `docs/AGENTS_RT64.md`, `docs/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md`, `docs/input-research/`, the untracked older reviews and `lib/rt64.7z`. Generated `CHANGELOG.md` stays with its release workflow.
