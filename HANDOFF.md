# Handoff

> Current state: 2026-09-06. See focused documents for detailed evidence.

## Current state

- Branch: `codex/rt64-modern-fog`. This run and preceding fog work remain uncommitted across the project, RT64 and N64ModernRuntime; existing user changes were preserved.
- **The reproduced enhanced geometry regression is fixed by withdrawing Zelda's unconditional camera-basis publication.** Fresh shaders alone did not fix it. The same modded Southern Swamp owl checkpoint became clean when the four camera EGBI commands were omitted.
- Final source removes the experimental perspective patch, float generation and precision switches. Original MM perspective/distortion/view behavior and existing interpolation tags remain. Generic RT64 float EGBI commands are restored to upstream behavior for mods (ADR-006).
- Two independent build defects are also fixed: missing shader include dependencies and stale generated patch-registration headers. Builds no longer intentionally require a second invocation to synchronize patches.
- Current executable: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
- SHA-256: `D6F7628E560797A20FCCF25FAA8CBF7F8B8244FF360215B8393527B0BEB46E51`.
- Complete Clang/LLD build and targeted runtime checks passed. Qualification is limited to recorded scenes/modes, not all effects/mods.
- **Both historical oracle directories are preserved.** `_working-directory/build-zelda-clang/Zelda64Recompiled.exe` is still the BAD oracle, not the new deliverable. Exact old paths/hashes remain in `docs/GRAPHICS_REGRESSION_HANDOFF.md`.

## Implemented in this run

- Withdrew the unsafe camera experiment; perspective must not be baked into affine model/world transforms. Future precision needs explicit camera representation and independent validation.
- Added missing identity sentinels to atmosphere's fixed-camera arrays and assertions that they align with projection indices.
- RT64 CMake tracks compiler-generated transitive dependencies for DXIL, SPIR-V, Metal input and embedded HLSL. Minimum CMake is 3.21. Windows Ninja/Clang tested; other platform builds not executed.
- Root CMake explicitly tracks generated patch registration headers and recompiler/config/symbol inputs. This fixes the reproduced blank-name mod-loader error after an incremental patch change.
- Added opt-in developer autostart, finite controller-read playback and Native presentation selection. Normal startup/input remain unchanged without environment variables.
- Added `tools/Prepare-RuntimeProfile.ps1` with independent profile copies, SHA-256 manifest and local-drive capacity preflight.
- Fog launch override now accepts `original` as well as `faithful` and `atmospheric`.

## Runtime feedback loop

Read `docs/RUNTIME_VALIDATION.md` and the native Computer Use skill. In this environment `node_repl` + `@oai/sky` works even though the browser CUA tool has no native support.

- Windows assets, controller mappings and portable configuration all resolve from **CWD**.
- Use a copied profile, explicit working directory, and interactive launch. Ordinary sandbox-launched windows were not targetable. Approved `require_escalated` / `Start-Process -WindowStyle Normal` worked.
- `ZELDA64RECOMP_DEV_AUTOSTART=1` starts the configured ROM.
- `ZELDA64RECOMP_DEV_INPUT=<JSON>` loads a finite controller-0 read sequence; it stays neutral afterward.
- `ZELDA64RECOMP_DEV_NATIVE=1` uses the actual F3 Native/RDRAM switch and logs backend/presentation.
- `ZELDA64RECOMP_FOG_MODE=original|faithful|atmospheric` selects the launch mode.
- Re-enumerate and bind the returned window after every restart. Mouse/capture worked; keyboard injection was unreliable. Window-control approval and foreground errors required recovery; do not infer success from an input call alone.
- Restore the stopped seed before each run. Playback is repeatable input, not deterministic state replay; clock, weather and animation can differ.

Local evidence: `_working-directory/diagnostics/2026-09-06/`, including manifests, source snapshots, disassemblies, build logs, `load-save.json`, `load-save-pause.json`, and `captures/`.

The existing isolated profile is `runtime/` in that directory; its copied candidate is named `tools-control.exe`. Check the manifest/hash rather than interpreting that filename as provenance. `Launch-Final.cmd` launches it with the correct CWD and without developer overrides. Test processes are stopped and the original seed saves/configuration/mod archives restored. Input opt-in intentionally disables interactive gameplay until restart.

Test seed: old build's portable profile, **47 mod archives and 46 enabled entries**, including an HD texture pack. This does not establish coverage of the user's entire broader installation. A second 3.5 GB copy exhausted disk; its incomplete output was removed, and the existing isolated profile reused. G: had about 450 MB free afterward.

## Evidence and qualification

- Bad oracle: captured radial wedges in attract characters, Clock Town/skybox and interiors.
- Fresh source control `75F9F676...`: newly compiled shaders still show wedges.
- Tools/sentinel control `B2B67524...`: copied save loads; Southern Swamp owl still shows wedges.
- Four-command omission `322DF33F...`: same mod profile/save/input loads and gives clean enhanced geometry. Its Native run logs Vulkan / Native-RDRAM and gives a clean low-resolution reference.
- Final retired-camera build loads the same mod profile; enhanced Original and Faithful owl views and Atmospheric pause inventory are clean. The same save also loads and renders cleanly with all external mod archives temporarily removed (built-in patches remain). Captures are recorded in the focused regression document. These are smoke checks, not matched-frame artistic fog qualification.
- Shader tests: no-op does no work; shared RDP header change rebuilds all five VS variants and DebugPS SPIR-V without rebuilding unrelated Bicubic/RenderParams outputs; RenderParams header change regenerates embedded text. All 109 shader hashes remain unchanged after adding dependency tracking.
- Patch tests: unsynchronized incremental build reproduced invalid hook addresses; explicit generated-header dependencies rebuild registration in the same invocation, and corrected candidate loads mods successfully.
- Playback tests cover read boundaries, neutral completion, opt-out and malformed/missing inputs. Profile tests cover copy hashes, save isolation, overwrite refusal and capacity preflight.

## Retained fog design

Original remains the persistent compatibility default and Native reference. Faithful evaluates the exact draw-local legacy fog response per fragment. Its clarity difference is caused by clamping after depth interpolation rather than interpolating clamped vertex fog.

Atmospheric redistributes authored optical depth into a height medium; it does not add another full fog curve. It requires matching world-camera semantics and environment fog signature. Local actor/effect and secondary cameras fall back to Faithful; mixed vertex fog falls back to Original.

Zelda publishes resolved LightContext fog, camera, sun, precipitation/storm and outdoor semantics into Workload metadata. Natural-sky normal rooms can remain outdoor while cutscenes suppress skybox drawing. Mods can mask overrides through `recomp_on_atmosphere_override`; see `include/z64recomp_atmosphere_api.h` and `docs/MODERN_FOG_MODDING.md`.

Existing calibration remains: base height share 22%, saturated-fog budget 3%, noise variation 12%, scattering 0.25, clear/wet far transmittance 90%/65%. Three along-ray noise samples and optical headroom limit dense authored fog redistribution. F1 tuning is session-local. Broad artistic requalification after the camera/build fixes is outstanding.

## Build notes

Local recipe: `_working-directory/diagnostics/2026-09-06/build-control.ps1`. It initializes VS, adds local tools/LLVM 19.1.3/Git, selects cached SDL and Windows SDK `mt.exe`; bundled LLVM manifest processing lacks libxml2.

Base-ROM generation still uses the corrected runtime recompiler; patch generation uses CI-pinned `a13e5cff...`. Generated patch sources are shared at repository root: do not build different patch candidates concurrently.

RT64 baseline remains integrated `5473732...`. Local submodule commits may not be public; publish to accessible forks only when requested. Upstream AI-assisted feature submission is not a project goal.

## Strongest next move

Expand the compact visual matrix beyond the completed modded/no-external-mod owl and pause checks: Clock Town/skybox/interior, dense opening fog, weather, transparency/water, framebuffer effects/Lens of Truth, distortion and high-refresh motion. Extend checkpoint state logging only where setup becomes a measured bottleneck.

Then prototype per-pixel evaluation of existing RSP lighting with Native/legacy A/B. RT64 already retains normals and lights, so test visible value before expanding MM light semantics or selecting shadows/GI. This run provides no evidence that an engine rewrite is justified.
