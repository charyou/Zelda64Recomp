# Handoff — RT+ pre-next-work-package evidence, 2026-09-25

## Current state

The checkout remains an uncommitted WP3 implementation on parent `a102424`, RT64 `613c418`, Plume `91e6711`. RT64 and nested Plume have pre-existing WP3 changes; this pass added only a focused CPU fixture and gated observational source/candidate diagnostics in RT64, local analysis scripts, and `docs/reviews/RT_PLUS_PRE_NEXT_WP_VALIDATION_2026-09-25.md`. The MM adapter, N64ModernRuntime, and renderer policy are unchanged. The parent sees a modified RT64 gitlink and this handoff/review/changelog update. No commits were made.

ADR-016 is the accepted RT scene lifetime/identity contract, with the ADR-015 amendment. WP3 currently owns one `RaytracingSceneRecord` per framebuffer, a full BLAS build per game frame, refits for later HFR occurrences of the same Workload, exact reuse for invariant content, and CPU-side identity/continuity/motion provenance. No temporal consumer reads that provenance yet. The original/Native/RDRAM path remains available. The prior WP3 implementation/build/runtime detail is preserved in the 2026-09-25 entry of `CHANGELOG-INTERNAL.md` and its referenced diagnostics.

## New decisive evidence

- **WP3 identity:** `rt64_wp3_identity_fixture` directly invokes production `RaytracingSceneRecord::describe/finalize` on distinguishable A/B triangles with equal Tagged inputs/topology. Stable A,B retains correct continuity. Reordering, inserting, or removing equal-input draws can preserve an ordinal's Tagged identity and report `Continuous` for the wrong logical surface. Without explicit vertex velocity, motion is `Unknown`; with velocity, the wrong correspondence is also `Interpolated`. Fixture CSV: `_working-directory/diagnostics/2026-09-25-wp3/wp3-identity-fixture.csv`. Current Tagged identity/motion must not be consumed as sufficient temporal correspondence without a separate history-validity/correspondence guard. No guard was designed or implemented here.
- **Intro source collection:** A no-input ROM autostart at Original refresh captured traced Workloads 104–8918, one complete 6,160-Workload attract cycle and part of the next. Population transitions at 1130/1674–1776/2229 recur at 7290/7834–7936/8389. Complete-cycle series: `_working-directory/diagnostics/2026-09-25-wp3/prewp-full-cycle-b0b05fc3-5836-439f-9a13-7a18fb25a9de/series.jsonl`. One traced framebuffer row per Workload was analyzed; zero dummy uploads were excluded. Scene transitions were separated from continuing-scene changes.
- **Semantic change (B):** In scene 108, room 0, a point at `(934,160,-460)` changes game radius `-1→730` at Workload 1776. The ensuing one→two source change is genuine semantic activation and materially changes local/GI candidate counts.
- **Binding opportunity (C):** In the same continuing scene, a game point at `(1146,240,-1712)` stays at radius 730 and constant RGB across the captured 2058→2059 boundary. Previously it has only unsuccessful bind attempts; afterward one verified bind/receipt occurs, two RSP source snapshots appear, and RT64 deduplicates them to one added collected value. No source cap was hit. Repeated no-input runs show the onset varies and can include a one-frame loss/return. In the indexed confirmation shot, the changing collected index is selected by **zero** current GI or unowned Direct pixels around the churn. Binding-derived population churn is real, but material current candidate/input churn is not demonstrated in this tested shot.

The report gives exact cases, frame windows, source values, capture paths, counts, classifications, and remaining ambiguity. It is the decision input; do not infer a new roadmap from this handoff.

## Build and validation

- Ninja targets `Zelda64Recompiled` and `rt64_wp3_identity_fixture` pass. Current diagnostic executable SHA256: `C0BC3E12C5BFCBC37B6E6A2ABC5B37D7639F3F40C74C6B01979CD8476AEAB8AD` at `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
- Vulkan runtime capture and repeated intro intervals pass without RT failure. No-series idle ROM autostart smoke run rendered RT+ and produced no series artifact. Enabled series logs about 5 KB per traced Workload plus material GPU atomic counter work (over 182,000 increments at one indexed Workload); it is diagnostic, not a performance benchmark. Disabled performance overhead was not measured.
- D3D12 runtime, Metal RT, and broader renderer certification are outside this pass. WP2 and WP4 were not implemented.

## Immediate architecture question

How should a future temporal consumer validate logical correspondence beyond the current ordinal Tagged identity and trusted motion? Source-collection expansion is a separate future ceiling; this local/GI shot does not establish it as a prerequisite for a meaningful temporal experiment.

Preserve the supplied untracked `docs/AGENTS_RT64.md`, `docs/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md`, `docs/input-research/`, the older reviews, `start-rtplus-dev.bat`, and `lib/rt64.7z`. ROMs, captures and binaries stay ignored in `_working-directory/`. `CHANGELOG.md` remains on its release workflow.
