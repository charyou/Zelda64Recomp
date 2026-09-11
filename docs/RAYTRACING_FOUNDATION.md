# Hardware RT foundation and directional hard shadows

Current: 2026-09-10 Run 2 finished; mandatory result checkpointed before UI. Vulkan hardware visibility and real Enhanced shadows visibly validated. Run 1 is preserved in git history; its primary-hit diagnostic remains independent. The incomplete historical `RT_ENABLED` renderer remains disabled.

## Direction and lighting ownership

Zelda's `patches/play_patches.c` publishes `envCtx.sunPos` through the existing environment bridge. `src/main/rt64_render_context.cpp` puts it in `AtmosphereParameters::sunDirection`; Application snapshots that metadata onto Workload. It is a world-space vector TOWARD the resolved sun, not a position requiring subtraction of the camera or receiver. The MM semantic reference `src/code/z_kankyo.c` draws the sun at `view.eye + sunPos`; its ordinary outdoor `light1Dir` uses the same sine/cosine vector at 1/25 scale, quantized to bytes. No sun direction is invented or reconstructed in RT64.

The vector can diverge from authored draw-local lighting, including indoor and original cutscene behavior. Production shadows therefore affect only actual directional RSP inputs whose normalized world direction agrees with normalized sun by dot >0.9998 (byte-quantization tolerance). Other directional terms and positional/local light semantics remain unchanged. This is a narrow compatible-direction contract, not a claim that every RSP light is sunlight.

## Files and exact flow

Paths below are relative to `lib/rt64`.

- `src/hle/rt64_workload_queue.h/.cpp`: independent default-off `rtShadows`, `rtVisibility`, existing `rtEnabled` primary state. Capability-gated union requests the existing presentation-time VertexProcessor/world position producer, including unmatched frames. DrawParams carries independent mode values.
- `src/render/rt64_framebuffer_renderer.cpp::addFramebuffer`: collects conservative executable opaque meshes and parallel `surfaces` records. Selects one perspective projection; sunlight consumers require published world-camera agreement using the same position/orientation tolerances as atmosphere. Primary-only diagnostics can still view their previous wider projection subset. Screen mapping includes RSP viewport scale/translation and the actual RasterVS scale/offset. RSPProcessCS negates clip Y and RasterVS negates screen Y again: both must be included, otherwise the mask is vertically flipped.
- `src/render/rt64_raytracing_debug.h/.cpp::record`: remains the small per-framebuffer owner despite its historical name. Builds BLAS/TLAS and traces BEFORE ordinary raster scenes. `composite` separately copies a diagnostic inset AFTER raster only when selected. Native never collects/uses this path.
- `src/shaders/PrimaryHitRT.hlsl`: primary intersection, surface reconstruction, sequential secondary visibility and independent barycentric/visibility diagnostic output. Camera rays cover NDC0 through0.99 as before; no promise of full far-plane coverage.
- `src/shared/rt64_framebuffer_params.h`: 48-byte FramebufferParams, with explicit padding and `float4 shadowSun` at byte32. XYZ normalized world sun, W production enable. RDPParams stays336 bytes. No raster-stage varying or linkage change.
- `src/render/rt64_descriptor_sets.h`: framebuffer set3 adds texture t3 `gDirectionalVisibility`, with valid dummy texture on disabled/failure paths. Each participating framebuffer binds its own result before raster; enable is zero on unsupported/empty frames.
- `src/shaders/RasterPS.hlsl`: full-resolution unfiltered Load at SV_Position.xy. Accept the mask only if call identity matches `instanceRDPParams` identity and primary clip W agrees with `1/SV_Position.w` within max(0.05,abs(W)*0.0001). Otherwise visibility=1. This rejects unsupported or overlapping raster receivers instead of darkening a final image.
- `src/shaders/PerPixelLighting.hlsli::shadePerPixel`: multiply only matching directional diffuse contributions by accepted visibility, before original light sum clamp and color combiner. Keep ambient, normal magnitude, shared-matrix equation, other light terms and all existing fallback semantics intact. Fog/blending execute normally afterward.
- `CMakeLists.txt`: isolated RT shader library targets lib_6_5 for GeometryIndex; shared raster outputs and wrappers regenerate through the existing corrected shader build flow.

Actual production flow:

```
Workload sun + presentation-time world positions + executable opaque ranges
-> multi-geometry BLAS / identity TLAS
-> primary surface hit + secondary ray toward published sun
-> full-resolution raw visibility / receiver identity / clip W texture
-> matching raster surface + matching directional diffuse contribution
-> original combiner, fog, blending, resolve and presentation
```

## Minimal reusable secondary-ray and Hit -> Surface contract

`SurfaceHit` payload is24 bytes: float2 barycentrics, hit distance, GeometryIndex, PrimitiveIndex, hit flag. `PrimaryHit` fills identity; `PrimaryMiss` clears the flag. `RaytracingDebug::surfaces[GeometryIndex]` is uint4: RDP call index, executable index start, face count, projection index. This is replay-local identity, not a temporal stable object ID. Positions and indices are the existing GPU buffers; indexed triangle vertices supply the geometric normal. No material database or Zelda actor/scene IDs.

`traceVisibility(origin,direction,maxDistance)` uses a separate uint occlusion payload initialized blocked, with `VisibilityMiss` clearing it. It uses FORCE_OPAQUE, ACCEPT_FIRST_HIT_AND_END_SEARCH, SKIP_CLOSEST_HIT_SHADER, mask255. Rays execute sequentially from ray generation, so recursion depth remains1. The SBT contains one raygen, two misses (primary index0, visibility index1), one primary closest-hit group; table currently256 bytes. A future AO ray can reuse origin/normal reconstruction and this occlusion convention without replacing the renderer. Richer surface lookup can start at RDP call/index identity already retained.

Origin offset is along the geometric normal oriented toward the outgoing sun hemisphere. Bias=max(0.05 world units, largest absolute world coordinate*2e-6), TMin0.02, sun TMax1,000,000. This is a small conservative initial bias, not a qualified solution for arbitrary coordinate scales.

RT descriptors: t1 AS, u2 RGBA8 debug output, t3 structured uint4 surface records, t4 structured float4 world positions, t5 structured uint indices, u6 RGBA32F visibility. Raygen push constants112 bytes: inverse VP64, sun16, screen16, modes16. Visibility texture stores (visibility, RDP call+1, clipW, primitive+1); misses store (1,0,0,0). Clip W is reconstructed from the primary segment's homogeneous endpoints. Output is raw and unfiltered, with enough receiver information to extend later signal processing; there is no history or temporal identity yet.

## Narrow caster/receiver policy

Casters retain Run 1 eligibility: nonempty executable indexed perspective triangles, depth-tested and depth-writing, ZMODE_OPA, pixel Z, alpha compare NONE, no coverage-times-alpha, clear-on-coverage or framebuffer alpha blending, in-bounds index ranges, no VertexTestZ-rewritten indices. Rectangles, raw/UI/orthographic, unsupported alpha/cutouts, coverage opacity and blends retain raster-only behavior. No broad material coverage expansion.

Opaque participating triangles deliberately cast from BOTH sides. This is an explicit thin-shell occlusion policy, not accidentally inherited raster winding or a claim to reproduce one-sided materials. TLAS disables face culling; opaque shadow rays do not request face culling. Raster receiver culling remains unchanged. The identity/depth check prevents a hidden/cull-rejected primary hit from arbitrarily shading another raster surface.

Receivers additionally require existing enhanced per-pixel-light eligibility, camera/projection participation, matching call/depth and a matching directional term. Original lighting, Native, unsupported lights/materials and unmatched pixels retain their old output. Whole eligible triangles cast, including their offscreen portions; raster scissor clipping is not reproduced in the AS. Only submitted geometry participates, so offscreen game-culled casters can be absent. Secondary consumers exclude inward/asymmetric custom clip ratios; symmetric outward guard bands are accepted. Multiple cameras/custom non-affine coordinate systems are outside qualification; do not broaden them with heuristics in Run 3.

## Resources and synchronization

Existing world-write -> BLAS -> TLAS -> ray-trace barriers remain. Surface upload gets a read barrier; raw result transitions GENERAL -> SHADER_READ before raster. Per-framebuffer resource lifetime and existing waited graphics fence protect reuse. BLAS/TLAS rebuild every enabled replay. Surface metadata upload is currently recreated per replay; no object-space cache/refit redesign. Diagnostics use separate RGBA8 output and the existing target-matched FullScreenVS/TextureCopyPS inset. Both outputs are full target size; the optional inset displays at half size.

No new Plume modifications were needed. Run 1's committed nested Plume fixes remain: per-geometry Vulkan BLAS ranges, queried scratch alignment, consistent TLAS flags and actual AS device address. Preserve them across updates.

## Focused verification and reproduction

Build: `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1`. Mandatory candidate SHA256 `DD9CE9427BE7BDBEB129E717A03B9FF3B5457D9FB27E556EA92FC5450792474F`. RT SPIR-V SHA `C54EEDE1F1135287D92432E9F2B8371F3CAF3366AE2EA9672E6C55BDCEB39398`; DXIL SHA `A74142DC67F9F5A42A17844423E1838F501F16EDF88EB7F2734EE3FA8E56237A`. Real objects and C/header wrappers generated, not just dependency files. Raster dynamic/specialized/library consumers also regenerated. `build-orientation.log` records the last correction build.

`_working-directory/diagnostics/2026-09-10-rt-shadows/run.ps1 -Mode off|primary|visibility|shadows` restores the existing copied seed, forces Vulkan/Atmospheric/per-pixel lighting/cutout-original, and reuses the finite972-read Town playback. Interactive execution boundary is needed. Never restore a seed or replace the executable while another copied candidate runs; launcher now rejects that condition. Stop with process exit wait. The original seed SHA remains `B12F0C6F5546C59DF8E9CD26970A81F8F7CD11803E9F7D4E2E13E6D03D8D1C9B`.

Root directly observed and saved `primary-town.png`, `visibility-town.png`, `shadows-town.png`, `off-town.png`: upright real geometry intersections; yellow visible walls versus blue occluded ground/Link/structures; no diagnostic inset in production; localized diffuse changes versus OFF with sunlit wall samples unchanged. Camera matches, animation/time does not match exactly. See HANDOFF for measured static samples. No severe visible corruption or device loss in these corrected checks. Feature-off has no RT initialization. Vulkan RX9070XT with real copied mod stack is a smoke test only; no validation layers, D3D12 runtime, MSAA, broad HFR/transition/mod or performance qualification.

The earlier candidate had a user-observed vertical flip, fixed by including RSPProcessCS's Y negation in screen inversion. Early runtime setup was also unreliable due to hidden concurrent instances after sandbox stop denial; temporary menu scripts are unnecessary with exclusive launch. Old Run1 PNG files in the earlier evidence directory are one-byte placeholders on this checkout; use the real Run2 PNGs rather than assuming those files are usable image evidence.

No optional secondary diagnostic or future-run feature was added. Run3 starts at `surfaces`, `SurfaceHit` and `traceVisibility` for Surface Classification + AO; retain original/Native compatibility, conservative fallback and the independently usable primary oracle.

## Final Graphics control, build and user verification

After the mandatory checkpoint, added persistent `GraphicsConfig::rt_shadows` (default false) in `lib/N64ModernRuntime/ultramodern/include/ultramodern/config.hpp`. Parent `src/game/config.cpp` serializes/loads/resets it; `src/ui/ui_config.cpp` binds Off/On and marks pending changes. `assets/config_menu/graphics.rml` adds one row with help/navigation and scroll overflow on the options column. `src/main/rt64_render_context.cpp` applies the setting to `WorkloadQueue::rtShadows` after setup and on configuration changes. Startup RT64_RT_SHADOWS overrides the saved setting only at launch; subsequent Graphics Apply takes effect normally. `src/hle/rt64_state.cpp` adds the independent session toggle beside per-pixel lighting in F1. Native and capability gates remain intact. No settings-system redesign.

The user manually verified the Graphics/F1 setting in the diagnostics runtime and closed it. Root does not claim a completed automated UI toggle test; injected Escape was ineffective and the user explicitly ended further UI verification. The final normal project build then succeeded from the finished working tree. Final executable SHA256 is `D8DF2EF071240676433F8DD02D14BE70B4CD3DF9651A8D645D85D4121F685CD8` at `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`; assets are synchronized. Same hash at the existing isolated profile's `rt-shadows.exe` and `finished-run2/Zelda64Recompiled.exe`. `finished-run2/final-build.log` and `final-hashes.txt` provide the final build record. The earlier mandatory-checkpoint hash above identifies the image-evidence candidate, not the final executable. Shared shader objects/wrappers were already regenerated and compiled; the final incremental build consumed them. No new runtime test after this final successful build, per user instruction.

Before finishing, preserved the pre-final known-good binary (SHA F88572810AADDD311110101DF635161B74F71AB8630557631E91E1938497FB21), existing PNG/log evidence and six supplied user reference screenshots under `finished-run2/`. The latter are reported with sun shadows and Atmospheric fog active; they are qualitative references, not matched A/B captures.

Known issues are deliberately left unresolved:

1. **Sunrise transient:** user observed sun shadows appear, nearly disappear and stabilize seconds later. User-provided research suggests MM CURRENT_TIME, skyboxTime, light-setting/RGB transitions and sun elevation are not fully synchronized. Plausible cause only, not a verified diagnosis. Run2's narrow matching-direction/receiver path also remains part of the system to examine later; no causal attribution is established here.
2. **Camera/view transient:** at least one camera movement into/behind nearby geometry temporarily collapsed shadows, then recovered. This is distinct from the sunrise report. No diagnosis or fix attempted.
3. **Coverage/art direction:** user finds many interiors without meaningful direct light and some open/open-roof areas behaving unlike normal sunlit scenes. A visible sky/opening alone does not prove a matching authored directional contribution or participation in the current subset. Do not label these all as one bug.

No further broad validation or renderer feature work is authorized by this handoff. `docs/RT_LIGHTING_VISION.md` records proposed future visual priorities separately from these implementation facts.


## Run 3 — Ambient / Spatial Lighting

Current source: 2026-09-11. Production AO/contact and first Environment/Skylight Fill are implemented, independently default-off. An initial narrow AO build was checkpointed before expansion. User F1 evidence found that first radius18 / 25%-ambient response barely visible; the expanded result supersedes it. Run-1/2 history above is preserved; this delta supersedes their FramebufferParams/result-texture details where stated.

### Shared scene, secondary queries and raw signals

- Same RaytracingDebug per-framebuffer owner, submitted opaque BLAS/TLAS, indexed world positions and 24-byte SurfaceHit. No second AS or material/actor/asset database. Primary identity and camera/depth rejection remain the Run-2 contract.
- PrimaryHitRT.hlsl::traceSurface(origin,direction,maxDistance) returns the nearest finite opaque SurfaceHit using existing primary hit/miss programs. Unlike binary traceVisibility it does not accept the first traversal hit. Geometry, primitive, barycentrics and distance remain available to later secondary consumers. Sequential queries preserve recursion depth1 and the256-byte SBT.
- Primary position uses barycentric world reconstruction. Degenerate geometry yields neutral results. Geometric normal faces the visible opaque shell for spatial rays; sunlight separately orients its origin toward the outgoing light hemisphere. Shared surfaceBias uses configured world-unit bias (default0.1) plus the existing coordinate-scale minimum; TMin0.02. Primary-only non-world projections explicitly disable all spatial flags.
- spatialVisibility uses12 fixed world-oriented cosine samples by default, configurable4–32, golden-angle azimuth. One nearest-hit set serves radius45 contact and radius300 environment enclosure. AO-only stops at the contact radius; fill expands the same queries to the greater range. Each signal averages saturate(1-hitDistance/radius), so distant obstruction fades rather than making a hard range boundary. No per-frame/pixel random seed, screen blur, history or denoiser.
- RGBA32F hit/visibility retains (sun visibility, RDP call+1, clipW, primitive+1); neutral (1,0,0,0). New RGBA16F spatial output stores (contact visibility, finite environment visibility, octahedral geometric-normal XY); neutral (1,1,0,0), validity from the hit record. RT descriptor u7 and framebuffer texture t4/space3 carry the new resource. Existing t3/space3 remains the hit/sun texture. Both receive explicit write/read barriers and framebuffer-local binding; disabled/failure/empty paths bind valid dummies with zero enables.
- TraceParams128 bytes, statically checked and within Vulkan's guaranteed push-constant minimum; appended float4 spatialTuning = contact radius, environment radius, sample count, bias. modes.w bits0/1 independently select AO/fill. FramebufferParams80 bytes: byte28 spatialFlags, existing shadowSun at32, spatialResponse at48, environmentFill at64. RDPParams336 and raster varying linkage remain unchanged.
- Raw signals/normal, independent hit depth and full replay-local identity form a backend-neutral boundary. No vendor types, reconstruction dependency or proprietary denoiser assumptions. No motion vectors, temporal IDs, world-history validity or complete G-buffer are claimed.

### Surface Response and ambient ownership

SpatialLighting.hlsli is the shared response implementation. RasterPS accepts spatial data only for matching RT call and clipW on Enhanced geometry. Sun visibility remains independent.

1. **Explicit ambient:** compatible per-pixel RSP lighting changes only actual bound ambient RGB before direct-light addition. Ambient-only sets are now accepted too. Smooth normal response uses the existing interpolated basis, with cofactor world-normal transformation for shared nonuniform matrices and geometric fallback for singular/zero normals. Original directional/source contributions remain separately owned.
2. **Authored fill:** conventional opaque texture-times-SHADE combiners can reserve min(SHADE*authoredFillBudget, resolvedEnvironmentAmbient). Spatial response adjusts only that reserved share before the original combiner. Default budget0.45. This extends grounding to eligible flat/vertex-colored scene geometry without pretending baked RGB is clean albedo or recoverable original ambient. It is an explicit artistic approximation; textures with baked lighting and opaque textured emissive-looking content may need user tuning. Primitive/environment-only emission, additive/special SHADE uses and unsupported combiners do not receive this inferred response.
3. **Unsupported:** missing geometry/camera/identity, alpha/blends/cutouts, raw/UI/orthographic, excluded clipping, incompatible response or Native retain existing rendering. No scene/actor/texture-name gating. Compatible mods receive the same automatic response; unsupported semantics remain raster fallbacks. Original actor blobs remain.

AO response = 1 - aoStrength*(1-contactVisibility). Environment response = (1 + upperLobeWeight*normal.y*environmentStrength) * (1 - environmentStrength*(1-environmentVisibility)). Combined ambient response is clamped to the configurable ambientFloor, default0.35, so contact and broad enclosure cannot compound into black. Exact direct light, shade alpha, unrelated combiner inputs, fog and final framebuffer are not AO-multiplied. Authored-fill geometry preserves at least1-budget*(1-floor) of its original SHADE at maximum darkening (70.75% with defaults); its approximation must not be described as exact direct/ambient separation.

### MM -> generic environment inputs

- Small internal bridge extension: patches/graphics.h::RecompEnvironmentFog grows29->31 words, checked in recomp_api.cpp. play_patches.c publishes LightContext.ambientColor, which is already the post-adjustment resolved ambient, and a broad upper-lobe hint from natural-sky configuration plus ordinary room behavior. Visual skybox hiding does not switch the lobe off during cutscenes. No time reconstruction, RGB energy model, MM IDs, local-source replacement or general environment redesign.
- include/zelda_render.h / recomp_api.cpp / rt64_render_context.cpp carry that data into AtmosphereParameters::environmentFill (generic RGB/lobe weight), snapshotted by the existing Workload publication. This reuses existing lifetime/ownership and leaves the mod atmosphere event unchanged. Ambient RGB preserves authored time/weather/zone palette in the color-only budget; explicit RSP ambient retains its actual draw-local palette.
- Natural-sky metadata shapes a broad artistic lobe, not an authoritative indoor/outdoor or sky-access classification. Skyless rooms use isotropic authored fill. A finite TLAS miss means only no participating obstruction within range: never open sky, never GI or physical skylight transport. Missing culled roofs/walls can under-occlude fill; publication alone does not repair scene completeness.

### Shared controls and defaults

F1 -> Game editor -> Lighting has independent **RT AO / Contact Grounding**, **RT Environment / Skylight Fill**, and existing **Experimental RT sun shadows** checkboxes. WorkloadQueue atomics feed DrawParams and the same production shader resources. F1 is session-local; applying Graphics options republishes persistent state.

| Persistent JSON / Rml binding | Default | Renderer range |
| --- | --- | --- |
| rt_ao | false / Off | On/Off |
| rt_environment_fill | false / Off | On/Off |
| rt_ao_radius |45|1–160 world units|
| rt_ao_strength |0.8|0–1|
| rt_environment_radius |300|32–1000 world units|
| rt_environment_strength |0.4|0–1|
| rt_spatial_samples |12|4–32, shader integer count|
| rt_spatial_bias |0.1|0.01–1 world units|
| rt_authored_fill_budget |0.45|0–0.6|
| rt_ambient_floor |0.35|0.2–1|

All numeric settings are persisted/reset/applied by the existing GraphicsConfig path and exposed by F1 sliders and Rml bindings. No separate preset system. Graphics RML rows, range controls, help and focus navigation were mechanically implemented by Luna; root checked the exact bindings/diff. Final visual Graphics-menu QA belongs to the user.

Launch overrides RT64_RT_AO=0|1 and RT64_RT_ENVIRONMENT_FILL=0|1 are independent of RT64_RT_SHADOWS. ZELDA64RECOMP_DEV_INSPECTOR=1 invokes the actual F1 inspector at setup when injected function keys are ineffective; developer_mode must be true. Mouse checkboxes were tested, not simulated substitutes.

### Build and evidence

Documented build-control.ps1 Clang/LLD build passed. run3-cluster-build.log shows actual regenerated RT DXIL/SPIR-V, C/header wrappers and compiled objects, plus dynamic/specialized/library raster consumers. run3-final-cluster-build.log passed after final defaults, ABI assertions, bridge and UI binding changes. Final executable SHA256 **963ACDEF7EF018A1C429B2FC1461DA4F0AAEBC9AE978AB42B08E6013DF7A0E45**. RT SPIR-V C1A2834AEBA14709C4ECA6D492EF3434C4929677F9F28B5A488220CBEC6095CC; DXIL 5BDDD3023ADE4D8A321109CB8B9FC15C78384C02EC882DC9AF18A3AD89A2CDD7. Artifacts and logs preserved under _working-directory/diagnostics/2026-09-10-rt-ambient/finished-run3/.

Root performed Vulkan RX9070XT checks using the existing copied mod profile, Enhanced per-pixel lighting, Atmospheric fog, original cutout-AA. Initial narrow pass had indoor AO-on and live nighttime Town on/off. Expanded cluster had actual independent F1 toggles on a paused Town workload, using the existing Debugger -> Pause (F4), with sun shadows enabled. cluster-paused-off/ao/fill/both.png preserve the A/B set. Scene/time are held, but post-blend noise/UI animation can still differ. No severe corruption or device loss observed.

Paused quantitative check: AO-only changed the40x42 post-contact region by mean RGB(-5.019,-3.109,-1.945), while open floor changed(-0.538,-0.312,-0.274), and sky was unchanged. This establishes localized production response beyond the earlier barely-visible pass. Fill separately brightened open floor and darkened enclosed shield surfaces. Those fill A/B captures use the earlier0.65 strength; final default reduced to0.4 after that evidence. See paused-pixel-check.txt. Final no-shadow attract smoke uses the reduced default; final captures/status are recorded in HANDOFF.

Limits: submitted-geometry and one-world-camera coverage, conservative alpha/cutout exclusions, no broad MSAA/HFR/D3D12 runtime qualification, no performance benchmark. Deterministic sparse rays can show angular bands, geometric-normal seams or motion changes as geometry/hemispheres move. No denoising claim. The known sunrise transient and separate camera/geometry sun-shadow collapse remain undiagnosed and are not masked or declared fixed by this work.
