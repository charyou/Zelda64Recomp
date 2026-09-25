# Handoff — RT scene lifetime / identity foundation (WP3), 2026-09-25

## Current state

Uncommitted, commit-ready changes on top of parent `a102424` / RT64 `613c418` / Plume `91e6711`. Nothing is committed in any repository:
- RT64 is modified, and its nested Plume is modified;
- the parent shows the `lib/rt64` gitlink as modified plus docs;
- N64ModernRuntime and the MM adapter are unchanged.

The durable decision is ADR-016, with an ADR-015 amendment. The basis is `docs/reviews/RT_PLUS_ARCHITECTURE_REASSESSMENT_2026-09-24.md` (WP3).

Implemented:
- **Scene record.** `RaytracingSceneRecord` (`lib/rt64/src/render/rt64_raytracing_scene.*`) is the per-framebuffer owner shared by all RT+ consumers. It holds surfaces, provenance, BLAS/TLAS and continuity. Membership is unchanged: exactly the submitted ranges.
- **Lifetime.**
  - One BLAS gets a full build (allowing updates) once per game frame (Workload submission).
  - It is refit in place for later HFR occurrences of the same Workload, i.e. the same buffers and ordered index ranges.
  - It is reused without any build when all content is occurrence-invariant and exactly unchanged.
  - The TLAS is rebuilt whenever the BLAS changes.
- **Plume.** A generic `updateBottomLevelAS` plus `allowUpdate`/`updateScratchSize` (Vulkan and D3D12; Metal RT remains unimplemented and is stubbed).
- **Identity and motion provenance.** CPU-side only; there is no GPU ABI change.
  - The stock TransformProcessor now records per-transform matching provenance (`DrawData::worldTransformProvenance`).
  - Surfaces carry topology, shape and content keys.
  - Identity is Tagged (group IDs, material key and ordinal), Content (exactly unchanged untagged geometry) or None.
  - Continuity and motion (Static, Interpolated, Unknown) are defined in ADR-016. Heuristic, ambiguous and new geometry fail closed but still render.
  - Provenance is computed once per game frame and reused by its HFR replays.
- **Requirements.**
  - `RaytracingRequirements` names every consumer and derives the unchanged trace modes.
  - Stock `raytracingEnabled` is again legacy-only.
  - `rtPlusPrerequisites` explicitly keeps frame matching and world vertices active: they are a real prerequisite of RT+ world positions.
- **Other changes.**
  - Persistent upload buffers replace per-occurrence buffer creation.
  - The RT camera falls back to unprocessed view-projection when projection processing did not run (previously an out-of-range read).
  - Developer series and benchmark samples report build, refit and reuse.
- **Removed.** The per-occurrence full rebuild and the per-occurrence surface/source/response/environment buffer allocations. A static/dynamic two-BLAS prototype was built, measured and removed (see ADR-016).

## Build

A full build passed: CPU, the Plume Vulkan/D3D12 backends, the patches, and the executable. PrimaryHitRT is unchanged; its SPIR-V/DXIL was regenerated during development, and the final shader source equals HEAD.
- Executable SHA256: `22ED66AC022F4B8FACEF7623B0FB4BBFE3D5CFB7A7D3FA348B68ACD1922052D2`.
- Path: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`; a copy is in `…/diagnostics/2026-09-25-wp3/candidate/`.
- Build script: `_working-directory/diagnostics/2026-09-25-wp3/build.ps1`.

## Runtime evidence (Vulkan, RX 9070 XT; `_working-directory/diagnostics/2026-09-25-wp3/`)

**Benchmarks.** Town noon, still, 16 samples, medians. The baseline is the HEAD executable in `baseline/`. Full numbers are in `performance.txt`.

| config | whole | AS build | trace | classification CPU |
|---|---|---|---|---|
| 144 Hz, Auto res, MSAA4X (product path) | 2.388 → 2.047 ms | 0.396 → 0.047 ms | 1.184 → 1.175 | 1.51 → 1.52 ms |
| 144 Hz, low res | 0.813 → 0.492 | 0.390 → 0.047 | 0.145 → 0.157 | 1.37 → 1.47 |
| 20 Hz, low res | 0.893 → 0.775 | 0.405 → 0.321 | 0.160 → 0.161 | 1.44 → 1.93 |

- At HFR, about 0.19 of samples are full builds (about 0.37 ms each) and the rest are refits.
- At 20 Hz every occurrence is a new frame (one build). The +0.5 ms CPU there is provenance and classification once per game frame.
- The 20 Hz AS reduction is observed but not attributed.

**Correctness.**
- **HFR motion series** (`wp3r-hfr-*`, `prod-visual-*`): exactly one full build per game frame, refits for the other ~6 occurrences, and zero within-frame topology changes. There are no RT failures.
- **Refit exactness.** A held Workload (FixedRepeat) gets 1 full build plus 8,842 refits with bit-identical trace counters.
- **Refit under motion.** Counter step distributions into full builds and between refits are identical (no boundary discontinuity).
- **Identity and motion.** Tagged and content identity stay about 99% continuous across frames. Motion splits into roughly 67% Interpolated (animated actors), 32% Static and 1% Unknown.
- **No-consumer and master off.** There is no RT initialization, no AS/trace pass and no scene work.
- **Native.** The presentation shortcut keeps the enhanced replay running underneath (pre-existing). The RDRAM path was not touched.
- **`bastian` fixture.**
  - Harness: `bastian.ps1`. It copies the "Bastian" slot into a run-local slot 1 and advances dialogs with periodic A.
  - The first playable Lost Woods clearing (around 250–270 s after launch) is equivalent between baseline and candidate.
  - It is underlit because the adapter publishes a warm upward primary with visibility authority 1.0 in this enclosed forest, so the traced sun is occluded by the canopy. This is a separate Source/Responsibility fixture, unrelated to WP3.
- **Device loss.** One device loss occurred, only in an early 20 Hz run of the removed two-partition prototype. It was not seen in about 20 launches of the final design.

## Limits / open

- **D3D12 runtime is unqualified.** D3D12 was requested but Vulkan was chosen (RDNA4 workaround). The new D3D12 update path is compile-verified only.
- **Not yet consumed.** Identity, continuity and motion are established on the CPU but no consumer reads them. There is no per-pixel motion/ID guide, frame seed or history, which are intended for WP4.
- **Refit scope.** Refits never cross game frames, so each new game frame pays one full build. Per-object instancing was rejected for now.
- **Not qualified this pass.** A local/GI interior case, MSAA snapshot capture and mod stacks were not specifically re-qualified; product-path MSAA4X runs rendered correctly.
- **Earlier limits still stand.** Off-screen culled actors do not cast (ADR-015). The secondary directional has no visibility realization.

## Next

**WP4:** a reconstruction seam and the first temporal backend on ADR-016 provenance. It needs:
- a per-pixel surface identity and motion guide derived from stock `worldVelBuffer`, only for trusted motion classes;
- history validity and disocclusion;
- a frame seed.

The Source/Responsibility work (WP2), including the `bastian` authority case, remains separate.

Preserve the supplied untracked `docs/AGENTS_RT64.md`, `docs/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md`, `docs/input-research/`, the older reviews, `start-rtplus-dev.bat` and `lib/rt64.7z`. The generated `CHANGELOG.md` stays with its release workflow.
