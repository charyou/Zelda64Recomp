# Handoff — RT+ temporal GI reconstruction (ADR-017), 2026-09-25

## Current state

The work is uncommitted, on parent `c44c6ce`, RT64 `eaf765a` and Plume `91e6711`.
- RT64: 12 modified files and three new ones (`rt64_indirect_reconstruction.{h,cpp}`, `IndirectTemporalCS.hlsl`).
- Parent: `src/main/rt64_render_context.cpp` (environment override) and documentation.
- Plume, N64ModernRuntime and the MM adapter are unchanged.

The final executable is `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`, SHA256 `4B25C20211D8A3B1E16075A6C7C7F20BEE0577B5A23B1E125D44A58F1B96E129`. A copy is in `_working-directory/diagnostics/2026-09-25-temporal/candidate/`. This is the exact binary that was benchmarked and qualified.

## What was decided and built

The authoritative decisions are ADR-017 and the ADR-016 identity amendment in `docs/DECISIONS.md`. The canonical contract and evidence are in `docs/INDIRECT_RECONSTRUCTION.md`.

**WP3 is closed with its implementation unchanged and its contract narrowed.**
- Tagged/Content identity and `Continuous` are candidate lineage.
- `Static`/`Interpolated` only make the stock motion vector admissible. They never validate history.
- Header comments now say so.
- `rt64_wp3_identity_fixture` still reproduces the false correspondence byte-identically. It is kept as regression evidence.

**First temporal consumer: bounded diffuse GI.** The raw signal is receiver-independent incident irradiance, so history is location-based.
- **Renderer:** RT64 `RaytracingDebug` produces a canonical 2.5D motion guide.
  - Per-surface WP3 admissibility is uploaded to the GPU. Stock `worldVelBuffer` covers Interpolated surfaces.
  - The camera is the one stored with the history occurrence.
  - Stock object motion is aligned to the stored occurrence: exact within a Workload; across the adjacent Workload, extended by at most half a game frame.
- **Backend:** `IndirectReconstruction` runs the unchanged spatial filter, then a temporal stage.
  - Per bilinear tap it checks stored-depth (3%) and normal (0.9) agreement. It never consults identity.
  - History ages with normal change and is clamped to the raw 5×5 mean ± 2σ.
  - Length is bounded by 6 game frames (6 occurrences at 20 Hz; the 15 cap at 144 Hz).
- **Unchanged:** composition, output contract and source handling.

The GI sample rotation varies per occurrence only while history is active. The session toggle (F1 "GI temporal history", default on) and `RT64_RT_GI_TEMPORAL=0` restore the exact history-free spatial path.

**Old WP2 (independent source collection) was not needed and is not started.** GI selects candidates per pixel and occurrence and uses no source identity. Resume WP2 for either of these:
- a consumer needing source lifetime or identity (source-associated reuse, source presentation, broad unbound local influence);
- temporal GI artifacts attributable to binding-driven population changes.

## Evidence

All on Vulkan, RX 9070 XT, Clock Town noon, deterministic playback. Scripts and runs are in `_working-directory/diagnostics/2026-09-25-temporal/`.

**Reprojection correctness.** The GPU motion guide matches an independent CPU re-derivation from the recorded matrices. Over a ~32 px/frame turn, the median error is 0.013 px and p99 0.03 px.

**Fast turn at Original refresh:**
- 79–93% of GI pixels reuse history.
- Reprojected frame-to-frame instability of the composition input: 0.0046 on the old path → 0.0016.
- Noise proxy: 0.0045 → 0.0027.

**Still shot:**
- Reuse 99.7%.
- Noise 0.0056 → 0.0023.
- Converged energy within about 1% of the spatial estimate.

**144 Hz series:**
- 1,166 of 1,171 occurrences have continuous history with aligned object motion.
- Within a Workload the scale is exactly 1.0. Boundary extension scales are 1.0–5.0.
- 4 boundaries were refused because the extension exceeded half a frame.

**Product images** (144 Hz, 1600×960, MSAA4X): on/off window captures look equivalent, with no ghosting or energy shift.

**Cost:**
- About +0.04 ms reconstruction at the product target (0.208 → 0.245 ms). Trace and whole-Workload are unchanged within noise.
- +12 µs CPU RT preparation.
- +32 B/pixel only while active (+49 MB at 1600×960).

**Smoke runs:** GI off, RT+ master off and Visible product runs are clean. A hidden-window `Auto` run initialised no RT; the cause is the hidden window, not this change.

**Design corrections found at runtime** (all recorded in ADR-017):
- The first spatial-result clamp darkened converged history by 3.5%. It was replaced by the raw-window clamp.
- A turning Link showed rotation-stale irradiance. Orientation ageing was added.
- An occurrence-count bound gave 0.6 s of lag at 20 Hz. It was replaced by a game-frame bound.
- Capture load triggers stock HFR frame skipping, which makes HFR image bursts unrepresentative. HFR alignment is therefore qualified from the uncaptured series.

## Known limits / open

- Radiometric change is only bounded. A moving actor's indirect occlusion lags on nearby receivers: 8–20% within about 50 units of a rapidly turning Link at 20 Hz. Light and source changes are likewise smoothed.
- Unqualified: D3D12 runtime, HFR image metrics, local-heavy source churn scenes, a second game, and dense deforming mods (where `Unknown` motion always restarts history).
- The pre-existing FullSync `DrawParams` path in `rt64_state.cpp` leaves several RT+ fields uninitialized. That is not new; the new fields now default safely.

## Next meaningful decision

A backend-evaluation work package: FSR Ray Regeneration on D3D12/RDNA4 against this baseline, using the input mapping in `INDIRECT_RECONSTRUCTION.md`. It needs D3D12 runtime qualification first. The only material gap is diffuse albedo, which admits a labelled constant proxy only.

Separately, decide whether the measured actor-shadow lag matters enough to warrant a faster radiometric-change signal. Candidates: a history-confidence output, or moment-based detection at more samples.

## Preserve

Keep the untracked `docs/AGENTS_RT64.md`, `docs/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md`, `docs/input-research/`, the older and red-team reviews, `start-rtplus-dev.bat` and `lib/rt64.7z`. ROMs, captures and binaries stay in `_working-directory/`. `CHANGELOG.md` stays on its release workflow.
