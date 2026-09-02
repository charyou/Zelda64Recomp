# Handoff

> Current-state document for the next development session.
> Update this file at the end of substantial work.
> Prefer current facts over historical narrative; Git contains the history.

## Current state

- Working branch: `codex/rt64-modern-fog`.
- The current fog-phase work is implemented but remains uncommitted in the project, RT64, and N64ModernRuntime worktrees.
- Original remains the compatibility default. The Graphics menu now contains a persistent `Fog Rendering` selector with Original, Faithful, and Atmospheric choices. F5 still cycles the live RT64 mode temporarily and prints the result to stdout; `ZELDA64RECOMP_FOG_MODE=faithful|atmospheric` remains a launch-time debug override.
- Final Clang/LLD Windows build succeeds at `_working-directory/build-zelda-clang/Zelda64Recompiled.exe`, including all DXIL and SPIR-V shader variants. The current 32,333,312-byte build was produced at 2026-09-02 16:19 local time and launched for testing.
- F1's Game editor has a nonpersistent `Atmosphere` tab with live controls and current-workload diagnostics. It displays the effective fog mode, MM fogNear/zFar, semantic strength, conservative outdoor confidence, and continuous weather strength.
- The newly tuned build is awaiting the user's final visual pass in the opening forest and following ordinary outdoor scene. The pause-menu corruption is already reported fixed. Do not begin Per-Pixel Lighting until the remaining fog pass is accepted or its findings are addressed.

## Implemented fog behavior

- Original is untouched and remains the Native/RDRAM compatibility reference.
- Faithful reconstructs the RSP NDC depth per fragment and applies the draw's exact legacy `mul`/`offset` response and fog color. It intentionally has no density multiplier.
- The visible clarity difference between Original and Faithful is understood: Original clamps fog at vertices and interpolates the already-clamped values, so a near vertex at zero fog spreads extra fog across a large triangle. Faithful interpolates depth and clamps the same linear response at the pixel. The difference is geometry-dependent historical vertex sampling, not a missing global scale.
- Atmospheric converts Faithful opacity to optical depth and redistributes a semantically gated part of that response into the physical height medium. It does not add an independent second fog curve on top of the complete legacy response.
- The authored physical scale comes from the reconstructed world-space span between the legacy zero- and full-fog endpoints. The low layer uses exponential height-density integration, not a second arbitrary distance curve.
- Zelda publishes MM's resolved `LightContext` fog RGB/fogNear/zFar plus `envCtx.sunPos`, `view.eye`, the world-camera direction/reference height, a conservative visible-sky/normal-room outdoor classification, current rain/snow, and storm/lightning state. `fogStrength = saturate((996 - fogNear) / 50)` reuses MM's own fog-influence signal from environment glare/lens-flare behavior.
- Height density uses a scene-scaled 1.5% zFar scale height clamped to 35–240 world units. A smooth semantic fog-strength gate keeps fogNear near 996 effectively clear.
- A semantic optical-headroom curve now reduces height redistribution again as authored fog approaches saturation. The user's two measured states calibrate one 69% morning control to about 2% effective blend at strength 1.0 (`fogNear 822`) and about 6.5% at strength 0.2 (`fogNear 986`). Base blend remains 22%; the saturated-fog budget is 3%.
- Two-octave world-XZ value noise now uses three samples integrated along the view ray instead of one endpoint sample. Drift is four times slower, default variation fell from 35% to 12%, and strong fog further damps its amplitude. Noise structures existing atmosphere but never decides whether a scene is foggy.
- Original `sunPos` also drives a subtle Henyey–Greenstein directional scattering response while MM's resolved fog RGB remains the in-scattering color.
- Classified outdoor world views receive a separate clear-air aerial-perspective floor even when MM fog strength is zero. It is expressed as Beer-Lambert transmittance at MM's resolved zFar (90% clear, continuously reaching 65% in fully wet/stormy air), uses a very small morning/weather height share, and combines with authored fog via `max` rather than addition.
- Rain and snow are normalized from MM's current resolved precipitation counters; storm/lightning supplies a 75% minimum weather strength. MM's resolved fog RGB already contains its environment/time/weather color decisions, so no second time-of-day tint curve is applied.
- The F1 live controls expose base/morning height blend, saturated-fog budget, scale height, density variation, directional scattering, and clear/wet far transmittance. They are intentionally session-local calibration controls; `Reset atmospheric defaults` restores the compiled values.

## Classification and pause-menu protection

- Atmospheric may replace only draws whose RSP fog coefficients and RGB match the resolved environment baseline. Local actor/effect fog remains Faithful; mixed per-vertex fog states remain Original.
- Atmospheric additionally requires an RT64 perspective projection whose inferred inverse-view camera position and +Z basis agree with MM's active world camera.
- This camera-semantic gate is required because MM's pause inventory deliberately creates its own perspective `View` around `(0, 0, 64)`. Applying world-space height/noise reconstruction to it caused the reported rainbow/banding corruption. Secondary/UI cameras now fall back to Faithful automatically without scene IDs.
- `docs/DECISIONS.md` ADR-002 and ADR-003 record the durable metadata and classification boundary.

## Validation and evidence

- User captures established a saturated opening case (`fogNear 822`, strength 1.0), an attractive moderate forest case (`fogNear 986`, strength 0.2), and an ordinary low-fog noon outdoor case. A manually useful morning value near 0.02 for the saturated case versus 0.69 for the moderate case motivated the continuous optical-headroom curve. The attached intro capture itself showed Faithful, so its sliders were inactive in that exact frame; the verbal Atmospheric A/B observation remains the calibration input pending retest.
- The user also captured pause-menu RGB/banding corruption present in only one fog mode. Decompiled MM confirms that the pause menu uses a separate forced-perspective camera, supporting the new world-camera gate.
- The user subsequently confirmed that the pause inventory looks correct with the camera-semantic gate.
- First testing of the new clear-air build reports clearly visible mode differences and a good normal outdoor result. Directional scattering has a visibly useful but art-direction-sensitive range; keep its default conservative at 0.25 until morning/noon and weather are compared. Rain/storm and the revised saturated opening fog still require coverage.
- The standalone shader generation pass succeeded for all modified DXIL and SPIR-V variants. Its later MSVC C++ phase lacked the Visual Studio include environment (`stddef.h`); this is unrelated to the changes and is superseded by the complete Clang application build.
- The complete Clang build compiled and linked the new environment ABI, graphics configuration, RT64 CPU code, and every shader variant successfully.
- `assets/config_menu/graphics.rml` parses as XML and all element IDs are unique.
- CPU layout assertions pass for the extended `RDPParams`: 320 bytes; modern fog at 128, atmosphere params at 160, sun at 176, camera at 192, live tuning/scattering/ambient at 208/224/240, and inverse view-projection at 256.
- `git diff --check` passes in the project, RT64, and N64ModernRuntime worktrees apart from expected line-ending notices.

## Required final visual pass

1. In the application Graphics menu select each `Fog Rendering` mode and Apply; close/reopen the menu once to confirm persistence.
2. Press F1 and use the `Atmosphere` tab. Confirm the rendered mode and note fogNear/semantic/outdoor/weather diagnostics for each comparison.
3. In the saturated opening compare Faithful and Atmospheric with reset defaults. Atmospheric should now stay close to the authored dense visibility while retaining only about 2% effective morning redistribution.
4. In the later forest (`fogNear` around 986), Atmospheric should retain the attractive stronger low layer without requiring a manual mode-specific morning value.
5. Continue to an ordinary noon outdoor scene. Atmospheric should now show subtle aerial perspective from clear-air transmittance while remaining mostly clear. Verify indoor rooms report outdoor confidence 0 and do not receive it.
6. If convenient, check rain/storm and one locally fogged actor/effect. Weather should increase ambient haze continuously; local fog must remain Faithful.

## Known limits and follow-ups

- This lightweight raster model attenuates visible surfaces using reconstructed world positions. It can produce convincing ground-hugging haze across terrain and objects, but cannot draw detached wisps in empty air. True free-volume fog would require fog geometry or a volumetric/froxel pass and is intentionally outside this phase.
- A froxel implementation is feasible in RT64 but is a separate multi-pass renderer feature: it needs a view-aligned 3D density grid, depth-aware integration/composition, temporal stabilization, and explicit handling for N64 transparency, secondary projections, and local per-draw fog. MM decomp semantics can drive injection and visibility targets but do not eliminate that renderer work. Do not begin it until the lightweight model's final visual decision is made.
- Camera-match tolerances are semantic and scene-independent but need the pending pause/cutscene visual pass. If a legitimate world cutscene falls back to Faithful, inspect its inferred view before loosening the gate.
- F5 is intentionally a temporary nonpersistent developer A/B control; the Graphics menu is the authoritative persistent setting.
- The environment bridge can retain its last valid Play state briefly outside gameplay. Fog signature plus world-camera matching now makes accidental Atmospheric selection substantially less likely; explicit lifetime invalidation remains optional hardening.
- Full regression coverage is still desirable for water, transparency, particles, Lens of Truth, framebuffer feedback, transitions, and high framerates after the required visual pass.

## Upstream separation

- The renderer baseline update is already isolated in project commit `0faf84a`, whose direct parent is current upstream `dev` commit `1a9c266`. It advances RT64 from `23cab60` to qualified upstream `5473732` and contains only the required Zelda API-adaptation edits. It can be cherry-picked or proposed independently of fog.
- Project commit `9b3d2a1` begins the fog integration and points RT64 at fog commit `05394e9`; later project/RT64 commits and the current uncommitted work are fog/RDNA4 qualification. The generic enhanced raster path, Zelda semantic bridge, and current experimental atmosphere can therefore still be split for review.
- RT64 fog commits `05394e9`/`c8ce62b` are not reachable from the configured upstream RT64 remote, and the current N64ModernRuntime branch/edits are likewise local. A superproject fog branch must not be published with unreachable gitlinks: first push those submodule commits to accessible forks or rebase the minimal feature onto accepted upstream commits. This does not affect `0faf84a`, whose RT64 target `5473732` is public upstream.
- For the original recomp project, upstream the renderer update first. Do not mix the still-being-qualified Atmospheric mode into that maintenance PR. Faithful Per-Pixel can later be proposed as a smaller separate feature, but its RT64 changes should be reduced to the conservative generic depth/fog evaluation rather than carrying Zelda's environment/weather adaptation.

## Build notes

- Full Windows builds use Clang/clang-cl. Add `_working-directory/tools`, LLVM 19.1.3, and Git's `usr/bin` and `mingw64/bin` to `PATH`, set `MSYS2_PATH_TYPE=inherit`, then build target `Zelda64Recompiled` in `_working-directory/build-zelda-clang`.
- Base-ROM generation still uses N64ModernRuntime's corrected N64Recomp; patch generation still uses CI-pinned N64Recomp `a13e5cff96686776b0e03baf23923e3c1927b770`.
- ROMs, generated recompilation sources, build products, captures, and diagnostics remain ignored under `_working-directory/`.

## Relevant checkpoints

- Project `0faf84a`: qualified RT64/Plume integration baseline.
- Project `9b3d2a1`: initial Zelda environment bridge and fog integration.
- RT64 `05394e9`: initial enhanced raster fog modes.
- RT64 `c8ce62b`: raster-linkage ABI preservation and RDNA4 Vulkan fallback.
- Qualified upstream RT64 baseline: `5473732a822a4423b5696e7cb18fecc425a59875`.
- N64Recomp `dfd4a2d` / N64ModernRuntime `222fbfd`: `osSetTime` runtime translation fix.
