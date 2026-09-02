# Handoff

> Current-state document for the next development session.
> Update this file at the end of substantial work.
> Prefer current facts over historical narrative; Git contains the history.

## Current state

- Working branch: `codex/rt64-modern-fog`.
- The current fog-phase work is implemented but remains uncommitted in the project, RT64, and N64ModernRuntime worktrees.
- Original remains the compatibility default. The Graphics menu now contains a persistent `Fog Rendering` selector with Original, Faithful, and Atmospheric choices. F5 still cycles the live RT64 mode temporarily and prints the result to stdout; `ZELDA64RECOMP_FOG_MODE=faithful|atmospheric` remains a launch-time debug override.
- Final Clang/LLD Windows build succeeds at `_working-directory/build-zelda-clang/Zelda64Recompiled.exe`, including all DXIL and SPIR-V shader variants. The current build was produced at 2026-09-02 15:36 local time and launched for testing.
- F1's Game editor has a nonpersistent `Atmosphere` tab with live controls and current-workload diagnostics. It displays the effective fog mode, MM fogNear/zFar, and semantic strength so visual A/B results can be tied to the actual environment request.
- The newly tuned build is awaiting the user's final visual pass in the opening forest and following ordinary outdoor scene. The pause-menu corruption is already reported fixed. Do not begin Per-Pixel Lighting until the remaining fog pass is accepted or its findings are addressed.

## Implemented fog behavior

- Original is untouched and remains the Native/RDRAM compatibility reference.
- Faithful reconstructs the RSP NDC depth per fragment and applies the draw's exact legacy `mul`/`offset` response and fog color. It intentionally has no density multiplier.
- The visible clarity difference between Original and Faithful is understood: Original clamps fog at vertices and interpolates the already-clamped values, so a near vertex at zero fog spreads extra fog across a large triangle. Faithful interpolates depth and clamps the same linear response at the pixel. The difference is geometry-dependent historical vertex sampling, not a missing global scale.
- Atmospheric converts Faithful opacity to optical depth and redistributes a semantically gated part of that response into the physical height medium. It does not add an independent second fog curve on top of the complete legacy response.
- The authored physical scale comes from the reconstructed world-space span between the legacy zero- and full-fog endpoints. The low layer uses exponential height-density integration, not a second arbitrary distance curve.
- Zelda publishes MM's resolved `LightContext` fog RGB/fogNear/zFar plus `envCtx.sunPos`, `view.eye`, and the world-camera direction/reference height. `fogStrength = saturate((996 - fogNear) / 50)` reuses MM's own fog-influence signal from environment glare/lens-flare behavior.
- Height density uses a scene-scaled 1.5% zFar scale height clamped to 35–240 world units. A smooth semantic fog-strength gate keeps fogNear near 996 effectively clear.
- A bounded height-medium blend defaults to 22% in strongly foggy conditions and rises continuously to 38% near dawn, derived from MM's positive-X, near-horizon `sunPos`. The previous additive 30%/52% formulation was the cause of excessive opening-scene fog and has been removed rather than hidden behind a density multiplier.
- Two-octave world-XZ value noise modulates existing low-layer density from 65–135% and drifts slowly. Noise never decides whether a scene is foggy.
- Original `sunPos` also drives a subtle Henyey–Greenstein directional scattering response while MM's resolved fog RGB remains the in-scattering color.
- The F1 live controls expose base and morning height-medium blend, scale-height/far-distance ratio, density variation, and directional scattering. They are intentionally session-local calibration controls; `Reset atmospheric defaults` restores the compiled values.

## Classification and pause-menu protection

- Atmospheric may replace only draws whose RSP fog coefficients and RGB match the resolved environment baseline. Local actor/effect fog remains Faithful; mixed per-vertex fog states remain Original.
- Atmospheric additionally requires an RT64 perspective projection whose inferred inverse-view camera position and +Z basis agree with MM's active world camera.
- This camera-semantic gate is required because MM's pause inventory deliberately creates its own perspective `View` around `(0, 0, 64)`. Applying world-space height/noise reconstruction to it caused the reported rainbow/banding corruption. Secondary/UI cameras now fall back to Faithful automatically without scene IDs.
- `docs/DECISIONS.md` ADR-002 and ADR-003 record the durable metadata and classification boundary.

## Validation and evidence

- User captures established a genuinely fog-heavy case (opening forest/cutscene) and an ordinary low-fog noon outdoor case. The noon case remains nearly identical across modes because MM resolves fogNear close to 996 there; this is the required low-fog behavior, not a morning-only implementation. The first additive Atmospheric height layer was visibly too strong in the opening scene and was replaced with optical-depth redistribution.
- The user also captured pause-menu RGB/banding corruption present in only one fog mode. Decompiled MM confirms that the pause menu uses a separate forced-perspective camera, supporting the new world-camera gate.
- The user subsequently confirmed that the pause inventory looks correct with the camera-semantic gate.
- The standalone shader generation pass succeeded for all modified DXIL and SPIR-V variants. Its later MSVC C++ phase lacked the Visual Studio include environment (`stddef.h`); this is unrelated to the changes and is superseded by the complete Clang application build.
- The complete Clang build compiled and linked the new environment ABI, graphics configuration, RT64 CPU code, and every shader variant successfully.
- `assets/config_menu/graphics.rml` parses as XML and all element IDs are unique.
- CPU layout assertions pass for the extended `RDPParams`: 304 bytes; modern fog at 128, atmosphere params at 160, sun at 176, camera at 192, live tuning at 208/224, and inverse view-projection at 240.
- `git diff --check` passes in the project, RT64, and N64ModernRuntime worktrees apart from expected line-ending notices.

## Required final visual pass

1. In the application Graphics menu select each `Fog Rendering` mode and Apply; close/reopen the menu once to confirm persistence.
2. Press F1 and use the `Atmosphere` tab. Confirm the rendered mode and note fogNear/semantic strength for each comparison.
3. In the opening forest compare Faithful and Atmospheric. Atmospheric should be lower-lying and spatially structured without the previous overall over-fogging. Tune the five controls live if necessary and record preferred values.
4. Continue to the ordinary noon outdoor scene. Atmospheric should remain mostly clear; a semantic strength near zero explains why the modes converge there.
5. Check one locally fogged actor/effect if convenient; it should retain Faithful per-draw fog rather than receive the environment height layer.

## Known limits and follow-ups

- This lightweight raster model attenuates visible surfaces using reconstructed world positions. It can produce convincing ground-hugging haze across terrain and objects, but cannot draw detached wisps in empty air. True free-volume fog would require fog geometry or a volumetric/froxel pass and is intentionally outside this phase.
- A froxel implementation is feasible in RT64 but is a separate multi-pass renderer feature: it needs a view-aligned 3D density grid, depth-aware integration/composition, temporal stabilization, and explicit handling for N64 transparency, secondary projections, and local per-draw fog. MM decomp semantics can drive injection and visibility targets but do not eliminate that renderer work. Do not begin it until the lightweight model's final visual decision is made.
- Camera-match tolerances are semantic and scene-independent but need the pending pause/cutscene visual pass. If a legitimate world cutscene falls back to Faithful, inspect its inferred view before loosening the gate.
- F5 is intentionally a temporary nonpersistent developer A/B control; the Graphics menu is the authoritative persistent setting.
- The environment bridge can retain its last valid Play state briefly outside gameplay. Fog signature plus world-camera matching now makes accidental Atmospheric selection substantially less likely; explicit lifetime invalidation remains optional hardening.
- Full regression coverage is still desirable for water, transparency, particles, Lens of Truth, framebuffer feedback, transitions, and high framerates after the required visual pass.

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
