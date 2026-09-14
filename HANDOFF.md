# Handoff

> 2026-09-13 — Run 4 semantic local-light pipeline implemented and targeted active-source validation completed. Final build/Vulkan smoke passed; checkpoint binary and paused A/B evidence preserved. Normal controller input after deterministic playback is fixed and confirmed by the user. Changes remain uncommitted.

## Current implementation

Run 3 sun/contact/environment remains. Run 4 adds a default-off generic semantic local-source path: MM binding receipts -> generic RSP-slot source annotation -> verified replacement of original terms -> per-pixel finite local response -> finite RT visibility. RT64 contains no MM actor/scene branches. Visible emitters remain separate; no bloom, synthetic sources or GI was added.

Read `docs/SEMANTIC_LOCAL_LIGHTS.md` for the implemented contract, ABI, response, controls and limits; ADR-010 in `docs/DECISIONS.md` records the ownership boundary. `docs/MM_LIGHTING_SEMANTICS_RESEARCH.md` is supplied authoritative research, already used; do not repeat it. `docs/RT_LIGHTING_VISION.md` retains the artistic baseline. Run-3 details remain in `docs/RAYTRACING_FOUNDATION.md` and ADR-009.

The adapter verifies both positional and reference-directional realizations using original MM bind functions, unchanged light-group snapshots and consumed frame-local receipts. Ordinary RSP loads/color edits invalidate annotations. Unsupported/modified/reused bindings retain original lighting. Raster removes only RT-accepted owned terms. Tagged positional sets preserve original interpolated SHADE for failed receiver validation. Other lighting responsibilities remain independent.

**Coverage is bound-source enhancement:** Link/NPCs, static props and supported positional-lit world draws can receive locals. World surfaces with no original local binding retain current direct lighting and any independently enabled Run-3 spatial treatment. This is not scene-wide injection of every LightContext source. Extending to unbound receivers requires an explicit contribution/absence contract or profile permission, not proximity guessing.

## Controls and reproduction

- F1 -> Lighting: RT semantic local lights; default-off persistent JSON `rt_local_lights`. F1 edits are session-local. No new preset/Graphics-menu UI.
- F1 lighting-view combo: shaded, ownership, influence, direct, shadows, fallback reasons. F6 cycles in developer mode; F1 controls were directly validated, injected F6 was unreliable.
- Launch: `RT64_RT_LOCAL_LIGHTS=0|1`, `RT64_LIGHTING_DEBUG=0..5`. Existing AO/fill/sun controls remain independent.
- Isolated launcher: `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-13-local-lights/run.ps1 -Local -Town -NearTorches -Inspector`.
- Launcher uses the existing copied mod/profile runtime under `2026-09-07-coverage/runtime`, final executable `rt-local.exe`. Close a previous isolated test first. Launch/stop uses the established interactive execution boundary.
- `near-torches.json` loads the current copied Town save and walks toward the stall/torch, then releases normal input after 1112 controller reads. Successful playback no longer holds controller 0 neutral forever. Malformed playback still rejects with neutral input. Unit checks passed; user explicitly confirmed the controller works.
- Developer-only `ZELDA64RECOMP_DEV_TIME=day,hour,minute` sets time once in normal gameplay, skipping title attract. Launcher defaults to day1,23:00. `RT64_DEV_INSPECTOR_LAYOUT=1` keeps the actual inspector inside the test window.
- Existing Debugger Pause/Resume holds renderer workload for local-toggle A/B. Per-pixel classification changes require a new workload; do not interpret their paused checkbox alone as an original-path test.

## Build and decisive evidence

Build: `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1` using the documented project-local LLVM19/VS/Ninja toolchain.

Final executable: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
SHA256: **E094DF88F91D5D79C9CCAE977852C70E133FACA9ACA5173158E40A54C8AE5CFD**.

Checkpoint: `_working-directory/diagnostics/2026-09-13-local-lights/finished-run4/` contains executable, build logs, runtime log and paused captures. `local-build11.log` is the final successful build; preceding logs include regenerated real RT/raster SPIR-V and DXIL variants. Vulkan RX9070XT runtime validated; D3D12 only compiled.

Targeted active-source results (AO/environment/sun off for isolation):

- Runtime confirms generic source ingestion (first logged source slot0, position -278/50/-801, range100, RGB1/1/1) and hardware RT pipeline startup. Actual torch receivers are separately visible in diagnostics; the first logged source is not asserted to be that torch.
- Deterministic near-torch route reaches a useful normal-gameplay nighttime state. Camera/character motion and dynamic flame/source state remained sane during approach; no corruption/device loss observed. No dedicated moving-light actor or HFR qualification.
- Paused ownership view shows Link, stall and pedestal amber. Direct view shows warm source contribution on Link/stall. Shadow response shows exposed yellow surfaces and blue/purple occluded pedestal, character and structure surfaces. This is source-weighted shadow authority, not raw binary visibility.
- `paused-local-on.png` / `paused-local-off.png` are matched workload captures. Local-on minus off mean RGB: Link crop (-3.430,-2.557,+0.059), stall (-11.575,-5.839,-0.044), pedestal (-2.594,-1.582,+0.665). Replacement can reduce original vertex-light energy; this is not additive double lighting. The 55,000-pixel unbound-floor crop is exactly unchanged. Coordinates/results are in `paused-pixel-check.json`.
- `paused-ownership/direct/shadows/fallback.png` explain accepted replacements, no-owned-source world draws and RT-ineligible cutout/awning portions retaining original rendering. Unsupported receipt/mod cases are conservative code paths, not an exhaustive runtime fault-injection matrix.
- After resuming with per-pixel lighting disabled, a fresh workload rendered correctly through the original lighting path (original-live.png). Re-enabling per-pixel lighting after the daytime torches disappeared showed no stale amber source ownership. Renderer pause freezes the workload, not game simulation/time.
- User's five supplied captures are preserved as `user-evidence-0..4.png`: an additional NPC/stall/pedestal active-source view and shaded scene with AO/fill/sun. They support coexistence but are not matched quantitative A/B evidence.

Original user profile was not used for game writes. Pre-run disposable save copies remain in `saves-before/`; do not restore them over subsequent user testing without reason. The isolated test process is stopped after validation; original user profile remains untouched.

## Repository and remaining limits

Run 3 was already committed at run start: parent2342a42, RT642986426. Run-4 edits are in parent, RT64 and N64ModernRuntime. Preserve supplied research, existing Vision/addendum/input-research changes, `lib/rt64.7z`, mods and all unrelated work. No submodule reset/update/replacement. Plume unchanged. Generated CHANGELOG.md remains owned by its release-note workflow.

Known Run-3 energy/readability and motion artifacts, sunrise transient and separate camera/geometry shadow-collapse are not claimed fixed. Current TLAS only sees submitted eligible opaque geometry; game-culled/cutout geometry can be missing. Sources update at simulation rate without temporal interpolation/denoising; original seven-slot selection limits availability. Nonzero source-radius sampling is implemented generically but MM defaults to point sources and soft-source runtime qualification remains open.

No indirect prototype: a trustworthy bounded bounce/color budget needs additional surface-response work, beyond this completed direct-light checkpoint. Future work should address demonstrated ownership/coverage or signal quality using the same source and RT scene, not repeat the supplied semantic research or build a second lighting renderer.
