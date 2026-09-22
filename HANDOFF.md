# Handoff — source responsibility after ADR-012, 2026-09-22

## Implemented contract

ADR-013 separates modern source permission from historical decomposition. Primary retains exact ADR-012 unique-slot replacement. A valid source can additionally influence existing supported, RT identity/depth-validated spatial receivers with zero historical matches when the adapter explicitly grants permission. Ambiguous matches and positional fallback remain conservative. Unknown authored contribution is never subtracted. Unowned response uses only ADR-012 gain-minus-one, profile permission, diffuse response, .15 scalar peak cap and scalar remaining-SHADE headroom. Source hue remains intact; SHADE is appearance, not albedo. Visibility blocks only this increment. No new ray class/pass, source-energy equation, GI transport, varying or buffer size.

MM adapter sets primary color W=.35; default-zero generic permission is safe for other games. FramebufferParams lightingAuthority.z carries effective permission; mode bit64 reuses spatial identity/normal guides. Locals already implement the same distinction and keep their response/exclusion math. Secondary has no published unowned raster permission/incremental contract and stays unchanged. Ambient retains independent confidence-weighted fill transfer. See docs/PRIMARY_ENVIRONMENT_DIRECT.md and docs/SPATIAL_LIGHTING.md.

Expansion defaults on within Enhanced, bounded by source/receiver permission; session RT64_PRIMARY_EXPANSION=0 or F1 Primary spatial responsibility restores ADR-012 alone. rt_primary_direct off / Direct authority0 also disables it. Views30/31 expose role/applied increment. Existing local/GI/fill/shadow controls remain independent.

F9 adds a session RT+ master atomic; RT64_RT_PLUS_MASTER=0 starts OFF. Effective DrawParams plus existing Enhanced VS/PS gates restore authored per-vertex lighting, original fog/cutout and disable directional visibility, local/spatial direct, AO/enclosure, GI/ambient transfer and diagnostics. Requested config/overrides are never rewritten. ON restores their exact values. Ordinary raster MSAA sample allocation, resolution/presentation and mod/texture replacements remain baseline settings: MSAA changes require application-wide shader/cache/target rebuilding, not a safe per-frame flag. F1/stdout confirmation exists. Physical key delivery remains unqualified (below).

## Build and checkpoints

Full integration build and final CPU link passed. Actual raster DXIL/SPIR-V dynamic/library/specialization/flat/MSAA and PrimaryHitRT consumers regenerated; existing CPU assertions and fresh DXIL reflection confirm 128-byte FramebufferParams and offsets80/96/112. Final executable SHA256: 177110D253382F75BADCEE606601E2C71A0E67BE1AA98A427EAEABC7695EC113.

Evidence root: _working-directory/diagnostics/2026-09-22-responsibility/. build-integration.log preserves shader compilation; build.log is final; raster-abi.txt is reflection. run.ps1 clones mutable profiles and uses existing playback/warp/held-workload capture. Architecture checkpoint and prior handoff are saved there. Changed code: parent adapter; RT64 queue/DrawParams, framebuffer renderer, shared parameter comments, primary helper, per-pixel/raster shader, F1/F9 and capture labels/fixture. No N64ModernRuntime change.

## Checkpoint 3 — focused qualification

- noon-compact-34a9bdd9-a702-4ad8-a121-c404171b3a00: all16 states captured. 3954 owned crop pixels; expansion on/off gives identical raster, GI, local, spatial and visibility buffers. Positive owned term matches gain1.69089 within3.1e-5 UNORM error. Authority0/.5/1 raster means .218635/.230973/.243107. No broader receivers in this crop.
- inn-e13777cc-e5e8-4eb5-9667-e8743f9f802d: all16 captured. 3393 broader pixels, 702 owned. 3148 blocked broader pixels have exactly zero increment. Visibility off exposes nonzero bounded response. Owned and unrelated lighting buffers unchanged. Expansion disable and master OFF->ON restoration are byte-identical after the queue snapshot correction.
- inn-clear-924b58f4-e822-4ab4-8c2a-aa90b3e83a53: same known Inn entrance0xBC00 with existing visibility-off control, all16 captured. 3456 broader pixels; 33 change final raster, max RGB delta .021561; max shader increment .074403. 640 owned pixels unchanged by expansion. Authority means .122949/.124204/.125298. Disabling expansion returns exact previous raster; Direct0 equals interpretation off; master OFF->ON restores exact configured raster. Raw GI/local/spatial/visibility unchanged by expansion. Contact sheets visually inspected; this is narrow composition evidence, not broad scene/seam certification.
- Clean16-sample Inn benchmark: whole workload median .68406ms, fused RT .17510ms, no incomplete samples or resource growth; no readbacks. No new traversal topology/resources. performance.json records this sanity check, not a speedup claim.

Final CPU-only correction denies RT mesh submission/initialization when no effective consumer exists (e.g. all spatial features off and Direct authority0). Previously qualified active configurations are unchanged. Final-binary master-OFF benchmark completed all16 samples with zero RT build/tracing/reconstruction intervals, no incomplete samples and no resource growth (master-off-final-* under evidence root).

## Limits / next qualification

Implementation/build and focused lighting/override-state validation are complete. Missing/invalid semantics, ambiguous duplicate matching, no-guide fallback, isolated mode64-only operation, Native, MSAA/cutout and fog visual endpoints are structurally verified, not individually runtime-fixtured here. F9 is wired before inspector input consumption with repeat suppression; injected F9, existing F1 and Escape all failed to reach the game through the available computer-use route. Do not claim physical hotkey/UI notification delivery was qualified. Next narrow manual check: press F9 twice in ordinary gameplay, confirm OFF/ON and restoration with mixed enabled/disabled features. The capture fixture now releases its controls after16 occurrences.

Initial192x160 burst dropped writer-busy states; superseded by complete64x64 runs. No broad renderer/mod/platform certification. Camera-motion/caster-submission/cadence/reconstruction limitations remain outside scope. RT+ broader coverage is bounded and enabled, not a claim of universally coherent outdoor lighting.

Changes remain uncommitted in parent/RT64. Preserve supplied untracked docs/input-research, docs/reviews, source map and lib/rt64.7z. Generated CHANGELOG.md stays with its release workflow; relevant release-note summary is in CHANGELOG-INTERNAL.md.
