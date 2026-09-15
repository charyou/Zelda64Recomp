# Lighting instrumentation contract

Status: **architecture accepted; implementation pending**, 2026-09-15.

This is a developer evidence system for the current RT+ path. It does not change lighting policy, source coverage, reconstruction, navigation semantics or rendering quality. Its success criterion is that a captured failure can be assigned to a responsibility, or explicitly left unresolved, without repeating a renderer investigation.

## 1. Decision and scope

Build three operations in the existing developer inspector:

1. **Capture** a labeled game/workload/render occurrence, its effective configuration, existing lighting buffers and bounded CPU evidence.
2. **Inspect** a pixel/region and draw, escalating to a small executed-ray ledger only when the existing evidence cannot distinguish causes.
3. **Measure** a bounded sequence with coarse GPU intervals and CPU timings, without diagnostic shaders, image readbacks or continuous logging.

Use a versioned local capture directory, JSON metadata/records, native-format binary buffer crops and PNG previews. No database, service, permanent telemetry stream, universal probe framework, actor registry or whole-game state serialization. Extend F1 Lighting; use existing launch/configuration plumbing for scripted capture requests. Collection is explicitly armed and bounded. A capture records facts and validity; an investigation note records hypotheses and decisions separately.

The useful causal chain is:

`game state -> supplied inputs -> accepted sources and geometry -> executed response/visibility/transport -> reconstruction -> composition -> image`

These are separate joins, not a single coverage score. A failed join is evidence of missing information, not evidence that a feature contributed zero.

### Evidence tiers

| Operation | Collected evidence | Cost boundary |
|---|---|---|
| Off | Existing renderer behavior; new diagnostics inactive | No diagnostic allocations, commands, GPU writes or file I/O; cheap disabled checks only |
| Armed capture | Bounded game snapshots, actual CPU decisions, configuration, correlations | Reuse existing traversal/classification; no repeated scans of meshes solely to count |
| Snapshot / short motion burst | Existing buffer copies and image; bounded ray ledger if requested | Explicit diagnostic run; report copy bytes, dropped frames and shader variant |
| Benchmark | Effective settings, workload dimensions, coarse CPU/GPU intervals | Production shaders; no images, detailed binding log, GPU ledger or inspector overlays |

Start with one snapshot or at most 16 selected render occurrences per burst, a 256 MiB outstanding capture budget and one writer job. These are developer collection limits, not renderer quality settings. Make limits visible and adjustable; exceeding one drops a capture with a reason, never stalls rendering to preserve telemetry. An optional metadata-only ring may retain 120 producing game frames while armed. Before-arm source history is unavailable. Do not implement an always-on retroactive GPU recorder.

## 2. Sources and checked implementation baseline

Authoritative current architecture: [SPATIAL_LIGHTING.md](SPATIAL_LIGHTING.md), [SEMANTIC_LOCAL_LIGHTS.md](SEMANTIC_LOCAL_LIGHTS.md), ADR-001/002/006–011 in [DECISIONS.md](DECISIONS.md). Build/runtime evidence remains in [HANDOFF.md](../HANDOFF.md).

The supplied [MM source map](input-research/MM_LIGHTING_INSTRUMENTATION_SOURCE_MAP.md) is the authoritative game research, referring to MM `486055ebec4c45f3020bfaa5d040698d6c2b85e4`. The requested repository copy was absent; this run copied the supplied Downloads file byte-for-byte. Its embedded citation tokens are provenance from the prepared report, not locally resolvable citations. No MM research was repeated. The recommendations in that report are research input, not extra user instructions.

Targeted code inspection used parent `e4cdbd5` and RT64 `b9bb4ca`. Follow-up implementation must record its actual component revisions, dirty status and built executable/shader identity; these hashes are context, not build pins. Historical legacy RT classes in this tree are not the current RT+ implementation.

### Implementation map

Paths below are repository-relative. Symbols are more durable than line numbers.

| Location | Relevant current behavior / diagnostic hook |
|---|---|
| `patches/play_patches.c::Play_Main` | Resets light receipts at entry; publishes resolved environment after update/camera hooks, before `Play_Draw`. CutsceneManager update later in the function is not part of that snapshot phase. Add the game snapshot here, with explicit phase. |
| `patches/semantic_lights.c::Lights_BindAll`, `owns_point`, `Lights_Draw` | 512 frame-local receipts; successful slot growth and actual realization verification; group equality check and consume-on-draw; emits immutable `GRAPH_ALLOC` source records into OPA and XLU. Observe these decisions rather than reimplement binding. |
| `patches/input_latency.c::Graph_TaskSet00` | Builds/submits the graphics task and blocks for completion in the ordinary patched graph path. Useful scheduling evidence, not a general metadata identity API. |
| `src/game/recomp_api.cpp::recomp_set_environment_fog`, registration in `src/main/main.cpp` | Patch-to-host environment conversion. Use an analogous developer-only snapshot handoff, copying fields rather than exporting game pointers. |
| `src/main/rt64_render_context.cpp::set_environment_fog`, `get_environment_fog`, `RT64Context::send_dl` | Mutex protects the latest global EnvironmentFog; `send_dl` reads it for renderer submission. It does not itself carry a producing-frame token. |
| `lib/N64ModernRuntime/ultramodern/src/rsp.cpp` | `SpTaskAction` / `submit_rsp_task` queue task data. Do not add MM diagnostic policy here. |
| RT64 `src/render/rt64_application.cpp::Application::processDisplayLists` | Copies atmosphere to the Workload before list processing. Preserve a copy of these **actually consumed** inputs. |
| RT64 `src/gbi/rt64_gbi_extended.cpp::setLightSourceV1` | Has the opcode PC before advancing to the second command; resolves payload and validates 12 floats. Capture annotation occurrence/acceptance here. |
| RT64 `src/hle/rt64_rsp.cpp` | Vertex loads snapshot lights; `modifiedColor` follows load/copy/edit. Diagnostic lineage must follow the same lifecycle, including light reload/color-edit invalidation. |
| RT64 `src/hle/rt64_state.cpp` lighting classifier | Builds `RDPParams.pixelLighting`; distinguishes set/mixed/transform/positional rejection and preserves unsupported paths. Existing `pixelLightingDraws`/`legacyLightingDraws` are draw counts. F1 Lighting lives here. |
| RT64 `src/hle/rt64_state.cpp::advanceWorkload`, `advancePresent` | Workload/present IDs exist, but are not sufficient to identify every interpolated render or displayed image. |
| RT64 `src/render/rt64_framebuffer_renderer.cpp::addFramebuffer` | Copies effective DrawParams, deduplicates sources, selects projection, builds executable RT ranges, receiver traits and tint. Primary CPU evidence hook. |
| Same file, `recordFramebuffer` | Calls `debugRT->record` before raster; `submitRasterScene` includes final lighting composition; `debugRT->composite` is only the optional inset. |
| RT64 `src/render/rt64_raytracing_debug.cpp::RaytracingDebug::record` | Current production RT+ resource owner despite its name: builds BLAS/TLAS, traces once, optionally reconstructs; owns existing buffers and failure state. |
| Same file, `Reconstruction::record` | One compute dispatch with raw/primary/normal guides and separate result. |
| RT64 `src/shaders/PrimaryHitRT.hlsl` | `PrimaryRayGen`, `traceSurface`, `traceVisibility`, `spatialVisibility`, `localVisibility`, `unownedLocal`, `indirectSources`, `diffuseIndirect`: exact ray/selection ledger hooks. |
| RT64 `src/shaders/IndirectReconstructCS.hlsl::CSMain` | Exact support/rejection and confidence calculation. |
| RT64 `src/shaders/RasterPS.hlsl::RasterPS` | Actual call/depth acceptance, fallback, ambient transfer, additions and final combiner. Diagnostic views must observe these locals. |
| RT64 `src/shaders/PerPixelLighting.hlsli`, `LocalLighting.hlsli`, `SpatialLighting.hlsli` | Authored normal basis/magnitude, directional matching, owned direct and ambient response. Do not duplicate these equations in a CPU explanation engine. |
| RT64 `src/shared/rt64_spatial_receiver.h` | Capability bits and `spatialCombiner`; retain separate caster/receiver/ownership decisions. |
| RT64 `src/hle/rt64_workload_queue.cpp::threadRenderFrame`, `threadLoop` | Actual configuration sampling, HFR occurrence, GPU submission/completion and existing timestamps. |

## 3. Correlation: freeze evidence, not mutable globals

### Identity hierarchy

Every record has a session UUID and schema version. Within it distinguish:

- **Game snapshot token**: monotonically assigned observation in a PlayState lifetime; also record a PlayState epoch and game frame counter. No pointer is a durable ID.
- **Display-list annotation occurrence**: task/list scope, command PC and execution ordinal. Reused memory or repeated list execution is not the same occurrence.
- **Workload ID** and its set of observed game tokens/spans.
- **Render occurrence ID**, previous/current workload identities and interpolation weights; workload ordinal within that occurrence; framebuffer-pair ordinal and projection index. Also record RT target size and raster viewport/scissor/screen mapping.
- **Present occurrence ID** when known, linked to render occurrences actually used. `workload.presentId` is a dependency, not a unique screenshot key. Skipped/generated/repeated occurrences remain distinguishable.

The selected image is captured from a named renderer output/target at a named stage. A desktop screenshot alone is only an approximate visual reference. Never attach the newest game snapshot to an older GPU result by wall-clock proximity. Wall-clock and monotonic times assist navigation, not joins.

### Chosen transport

Use an **opaque developer annotation in the executed display list**, interpreted generically by RT64, to reference a host-owned immutable game capture. Allocate the unused extended-command encoding during implementation; do not overload `gEXSetLightSource`, change its 48-byte record, or encode a token into a lighting float. This annotation transports no rendering policy and changes no transforms.

At the pre-draw publication point, copy the game/environment snapshot and reserve its token. Emit the marker before instrumented draw streams execute; OPA/XLU require either a verified common root marker or explicit markers per stream. Finalize binding evidence after drawing and publish it before task submission. A small graphics-task finalization hook should verify this ordering. The parser records marker scope and execution order in Workload diagnostics. On task/list scope entry start unassociated; no inherited token from the previous task. Nested execution follows the active span; explicit token changes open new spans. If several tokens occur, preserve the spans instead of selecting one arbitrarily.

Do not retain game/RDRAM pointers for later reads. The host store owns copies until parsing has completed and the associated capture/workload references have expired. Capped storage failure yields `snapshot_unavailable`, never substitution by latest state. Mark incomplete/missing/unknown marker spans; mods bypassing this path still render normally.

**Why this mechanism:** the current mutex gives a coherent latest environment, and `Graph_TaskSet00` blocks in the ordinary path, so a mismatch has not been demonstrated. Nevertheless that is weaker than an explicit join under mods, non-Play tasks and future scheduling changes. A token in the consumed stream proves which observation the producer associated with it, without adding MM payloads to generic runtime task APIs. Validate marker placement on actual executed commands before calling the join exact. This is a narrow diagnostic exception to the normal frame-global metadata bridge, not a replacement architecture for production environment publication.

Keep **game-published snapshot** and **Workload-consumed atmosphere** separate and compare them. Do not silently repair the production bridge during instrumentation. Report mismatch as a bridge/correlation problem. An exact token proves association with a publication phase, not that no later game code mutated a light. Binding snapshots supply draw-time values; capture a small post-draw environment comparison while armed to flag later mutation. Outside Play, game fields are unavailable and renderer facts remain valid.

### Effective configuration

Record actual `DrawParams`, `TraceParams`, `FramebufferParams`, environment values, relevant feature enables and fallback/initialization status for the selected occurrence. F1 values are sampled during rendering and may differ from JSON/startup config; multiple atomics are read separately. Save the actual combination used and optionally a configuration-change generation, not an invented atomic transaction. A/B requests must latch a configuration at an occurrence boundary and verify it in the output.

Include backend, device/driver, resolution/scaling/MSAA, HFR/interpolation/adaptive skipping, pacing/VSync/FPS limits, diagnostic view, raw bypass, fog/lighting modes, shader variant/cache warm state and component/build hashes. Save the copied profile/mod names, versions/order where available and a configuration fingerprint; unavailable version/hash stays unavailable.

## 4. Minimum game evidence

Capture this small snapshot once per producing Play frame **while armed**, not once per HFR output. Values are raw game fields with units/enums documented alongside friendly labels. Do not infer an environment setting index that is not stored by the game.

| Group | State and purpose |
|---|---|
| Location/lifecycle | `sceneId`, `sceneLayer`, current saved entrance, `curSpawn`, requested `nextEntrance`/transition state; current and previous room numbers, segment-valid booleans, load status and room `enablePosLights`/environment behavior. Separates actual state from requested warp and two-room transitions. |
| Mode/cinematic | Active gamestate validity, `gameMode`; `csCtx.state/curFrame/scriptIndex`, saved cutscene index, `CutsceneManager_GetCurrentCsId()`, `Play_InCsMode`; opening index if readily available. Title attract is a PlayState with title mode, not ordinary gameplay. |
| Clocks/weather | Raw day and CURRENT_DAY, CURRENT_TIME, `skyboxTime`, scene time speed; `gWeatherMode`, storm request/state, precipitation/lightning values already relevant to environment. Never merge both clocks. |
| Environment selection | `lightMode/lightConfig`, next config/change enable/timer/duration; settings-mode current/previous setting, override, blend enable/value/rate/override. Record all as game fields, label their mode-specific meaning. |
| Environment values | `envCtx.lightSettings`, adjustments, final `lightCtx` ambient/fog/near/far, final `dirLight1/2`; published generic environment/profile values and override mask; `sunPos` separately. Distinguishes authored preset, adjustment, adapter and renderer input. |
| Spatial/reproduction | Active game camera eye/at/up or equivalent available values and player position/orientation; actual renderer matrices are recorded separately. Save/checkpoint reference, input sequence reference and requested developer actions. Camera observations are not renderer motion vectors. |

The resolved colors and directions are authored game values, not radiometric measurements. CURRENT_TIME can drive direction while skyboxTime drives color near dawn. Visual sun position can lag in a cinematic. Environment directionals are also nodes in LightContext; do not count them twice as local point lights.

Take a bounded LightContext list snapshot at the publication phase. Record ordered node occurrences, type, point position/radius/RGB/glow or directional direction/RGB, and explicit environment-1/2 pointer equality **evaluated now**. A nonpositive point radius is present but not positive-range; do not label every node active illumination. MM's native pool is 32, not a safe universal mod traversal limit. Use a defensive cap and visited-pointer/cycle check while copying, publishing `truncated`/`malformed`; never assume an arbitrary mod list has 32 elements.

List snapshot membership is not a guarantee about every later bind. The bind ledger copies parameters at the binding event, allowing draw-time updates or additional sources to appear without a false missing-source conclusion.

## 5. Source and binding evidence

### No universal source identity

Use capture-local source occurrences and exact observed pointers only within their lifetime/snapshot. Do not expose raw pointers as cross-frame identity. Generic actor/effect/static provenance is UNKNOWN: LightNode/LightInfo have no owner. Do not add actor-by-actor hooks or creation-stack tracing for this contract. If later work needs durable source lifetime, it may add insert/remove generations; polling pointer equality cannot establish it and is unnecessary for this first system.

The existing adapter recovers a verified association that the original `Lights` structure loses. Preserve that association in a **CPU diagnostic sidecar**, not in production SemanticLight equality:

1. At each `Lights_BindAll`, assign a bind ordinal; copy refPos presence/value, positional-mode decision, initial/final slot count and source attempts in list order.
2. Observe before/after `numLights`, attempted source parameters, returned slot and `owns_point` result. Distinguish not bound, bound-unverified, bound-verified and unsupported type. If the binder returns no reason, record `not_bound_reason_unobserved`; do not infer offscreen/range/slot exhaustion merely from no slot increase. Record pre-existing full-slot state as an observed condition, not a claimed sole cause under replacement binders.
3. At `Lights_Draw`, record receipt found/equal/consumed, invalidation/reuse/overflow and actual annotated slots. Emit sidecar association keyed by token, normalized RDRAM address of the first annotation opcode and expected command/payload bytes. OPA/XLU emissions are separate commands referring to the same binding evidence.
4. `setLightSourceV1` records executed PC/ordinal, actual payload and acceptance. Associate only if token and bytes match the sidecar. Copied/replaced mod commands remain unassociated; approximate color/position matching is forbidden.
5. Carry optional CPU lineage alongside RSP light snapshots through loads, copies, edits and equivalent-set handling. Preserve contributing lineage lists when equivalent values merge. Zero lineage means unassociated, not invalid lighting.
6. At `addFramebuffer`, record exact SemanticLight dedup groups, snapshot origins and cap outcomes. Current dedup is byte equality across the complete struct; different runtime nodes with equal values may collapse and one changing node may produce several value snapshots. Name the result `published_unique_value_snapshots`, not unique physical lights.

Detailed attempts are capture-only with a fixed record cap and dropped count. Aggregate observed outcomes while traversing once; no stderr per event. Diagnose a selected source using the ledger rather than collecting all producer metadata indefinitely. Existing 512 receipt overflow and renderer cap64 are separate counters. The upload dummy inserted when the source vector is empty is **not a published source**; count before insertion.

### Source questions this answers

Existence -> binding -> receipt -> emitted annotation -> accepted RSP snapshot -> published value group -> receiver selection -> response is a chain with evidence at each step. Nonparticipation is not one boolean. An apparently glowing object absent from LightContext and binding evidence establishes only that the observed paths supplied no source; it does not establish that the game/mod has no other lighting path or authorize synthetic emission.

Unowned direct currently excludes a bound source using position/range and color/strength equality, **not** full response equality or stable source identity. Preserve this exact predicate and report it. Its four ranked candidates and stable tie order are distinct from the two GI candidates selected once per primary receiver. `response.w` authorizes unowned direct; local bounce permission is `environment[4].x` and is a different responsibility.

## 6. Geometry and receiver evidence

At the existing CPU gates, record a compact decision row per submitted candidate/range on captured workloads:

- Workload/framebuffer/projection/call key and executable index range/triangle count.
- Perspective/indexed/nonempty/viewport/scissor/guard-band checks; opaque depth/blend/alpha checks; index bounds; world-camera match distance/agreement and selected projection.
- Original-light classifier result and evaluated rejection reason; modified-color provenance; unlit/authored-lit classification; combiner eligibility; independent capability bits; actual draw tint proxy.
- AS inclusion and receiver capabilities separately; RT unavailable/failed/empty-scene status separately from semantic rejection.

Observe booleans at the owning branch. For a short-circuited gate, mark later predicates `not_evaluated`; record the first executed rejection. Do not create a second classifier that drifts from production. A call can have multiple executable ranges; count **ranges** and triangles, not objects. Provide per-workload original lighting draw counts and per-framebuffer RT-range counts with their explicit denominators. These populations are not interchangeable and are not screen coverage percentages.

This table covers submitted work only. Missing/game-culled geometry cannot be counted as an excluded submitted draw. No total-world coverage claim is possible. Inspector selection can show an excluded raster draw even when it has no RT primary hit. Do not require a valid RT hit to inspect fallback.

For a pixel, separately show CPU capability and raster acceptance. Current raster match is call+1 equality and `abs(hit.clipW - rasterClipW) < max(.05, abs(rasterClipW)*.0001)`. Primitive+1 is available in the RT guide but is **not** part of that production match. A primary RT hit alone does not prove the final visible raster fragment, especially near silhouettes, overdraw, transparency or MSAA.

## 7. GPU evidence: existing buffers first

Capture copies at the selected framebuffer/render occurrence after the resource's producer, before it is overwritten/reused. Use region copies into owned staging buffers, existing completion, then a background file writer. Buffer crops retain exact storage bits, row pitch and format; previews are derived and explicitly labeled. No synchronous copy-and-wait path and no full-frame dump every frame.

| Current resource | Exact meaning and limitation |
|---|---|
| `visibility`, RGBA32F | X primary sunlight visibility; Y call+1; Z reconstructed clip W; W primitive+1. Zero Y means no valid primary surface guide. Clip W is not linear camera distance. Float IDs are unfiltered and replay-local. |
| `spatial`, RGBA16F | XY contact/environment finite visibility, ZW octahedral geometric normal. These are finite nearest-hit weighted signals, not sky visibility or physical irradiance. |
| `local`, RGBA32F | XYZ primary world position, W bit container: seven 2-bit visibility values, ownership bits21–27, bit30 finite-float guard. Decode W as bits; never average/filter it. Valid only for the relevant enabled/accepted path. |
| `spatialDirect`, RGBA16F | RGB unowned direct already capped at .3; W receiver traits. It is not uncapped source energy or owned direct. |
| `rawIndirect`, RGBA16F | RGB bounded four-sample transport; W mean of four bounce distances, misses replaced by radius; W=-1 means invalid. A hit on a nonparticipant still enters distance mean. Zero RGB is not invalidity and mean W is not a GI boundary distance. |
| `reconstruction.result`, RGBA16F | RGB filtered transport; W support confidence, -1 invalid. Different meaning from raw W. Raw bypass changes composition selection, **does not skip reconstruction dispatch**, and confidence still comes from reconstruction. |
| Optional primary inset `output`, RGBA8 | Diagnostic barycentric/visibility view, not final beauty lighting. |

The reconstruction guide's identity is used for nonzero validity, **not equality across neighbors**. The shader tests raw validity, normal agreement >=.9 and relative clip-W difference <=.025, over a 7x7 lattice at spacing2. Weight is `exp(-(x*x+y*y)/8-relativeDepth*150)*pow(normalAgreement,32)`; confidence is `saturate(weightSum/12)`. It is support weight, not transport accuracy, history confidence, ray hit fraction or a probability. Export guide buffers with their formats so guide quantization can be distinguished from input changes.

Existing views0–9 remain useful navigation: ownership, local influence/direct/shadow, fallback, receivers, spatial local, raw/reconstructed. Radiance views multiply by4 and saturate; they cannot measure energy. The current reasons view also combines several cases. It must not be used as a complete reason taxonomy.

### Small additions that are necessary

**Executed-ray ledger:** an opt-in diagnostic shader variant writes one fixed record per selected RT pixel (default up to 16 picks, or a capped small ROI). Ray generation owns the record, so no atomics or fragment races. Record values at the actual production computations, sharing the same functions; do not run a different CPU ray model. Diagnostic variants are never benchmark results.

For each pick store primary ray origin/direction/TMin/TMax, hit geometry/primitive/barycentrics, position/geometric normal/bias, source candidate indices and scores, plus:

- Four bounce samples: actual direction and limit; hit/miss, distance, surface traits and normal validity; acceptance/skip branch; tint; ambient/primary/secondary/local incident components, before/after clamps and distance fade. Mean output must reconcile with raw storage within format precision.
- Up to four unowned direct selections: bound exclusion, finite relevance, cosine, attenuation, visibility-query-executed flag, visibility/shadow authority, pre-cap sum and post-cap result. Candidate inspection for a specifically selected rejected source may show its actual score/exclusion without logging all candidates at every pixel.
- Two selected bounce-light indices and per-bounce evaluation/skip/occlusion results. Selection is per primary receiver, not re-ranked at the bounce.
- Contact/environment query limits and contributions, and primary sunlight query executed/result. Distinguish a skipped visibility test/default1 from traced-visible1.

Visibility uses accept-first-hit and skips closest-hit; it returns occluded/not-occluded, **not nearest blocker identity or distance**. Do not change that production query for diagnostics. If a blocker must be inspected, a separately labeled diagnostic closest-hit query on the retained scene is permitted; it is supplementary and may pick a different hit under ties. Closest-hit bounce rays already have surface identity. The ledger must record `miss_in_submitted_scene`, never `outside_world` or `open_sky`.

**Reconstruction explanation:** for a selected pixel derive the tap acceptance and weight from the captured raw/guides using the same equation, or instrument the existing compute invocation with a unique pixel record. Prefer the former first. Label CPU recomputation and check its RGB/confidence against captured GPU output within format tolerance. Summarize rejected taps by executed branch and accepted weight; full49 tap details are on-demand only. No full-screen rejection counters.

**Raster composition views:** extend the existing diagnostic output with actual geometric versus shading normal/magnitude, raster match/fallback, raw validity/confidence/effective authority, current ambient/fill, replacement ambient/fill, preserved direct, owned local, unowned addition and pre/post-clamp SHADE. Expose terms from existing shader locals/helpers, not independently reconstructed equations. Use repeated views of a held workload with fixed interpolation/configuration and disable selection highlight/ubershader tint. Store the view and gain; these are visual diagnostics, not calibrated framebuffer energy or unique fragment records.

Do not add a per-pixel raster UAV “last writer” ledger: overdraw/MSAA make it ambiguous. Exact numeric fragment attribution beyond RT buffers is deferred; if a future decision needs it, use an isolated depth-tested diagnostic replay/export with a demonstrated fragment/sample identity contract. Until then, ambiguous raster pixels are labeled as such and investigated with draw isolation and matched images. No raster varying ABI expansion is needed for this initial system.

## 8. Composition interpretation

The instrument observes implementation, not an idealized lighting equation:

- `explicitAmbient` depends on `usePerPixelLighting`, including smooth shading and positional enable. CPU authored-lit traits do not guarantee this runtime branch.
- Effective indirect authority is clamped requested authority times reconstruction confidence when valid receiver/GI gates succeed. Raw bypass still uses that confidence.
- Authored-lit ambient is an explicit RSP term. Authored-color's .75 fill / .25 retained artwork is an artistic partition. Neither is physical albedo recovery.
- Replacement uses contact/enclosure, a .18 ambient floor, and `min(indirect*1.8, ambient*1.5+.1)`. These are bounded authored-energy conventions.
- Owned local terms remove only accepted slots, keeping the remaining direct terms. Primary directional visibility requires direction dot>.9998 and RGB difference<.008, excluding semantic locals. Secondary is not automatically shadowed as primary.
- Unowned local gain changes from1 to2 with requested authority; additive budget changes .3 to .45. Thus an authority A/B is not exclusively an ambient experiment, including when GI is off but spatial locals are on.
- The residual additive GI scales by the untransferred share. SHADE clamps, material combiner, fog, blending and later presentation still affect final pixels. Lighting intermediates do not add linearly to screenshot RGB.

Record these evaluated terms/gates where observed. A dark original value does not alone establish intentional art direction; it could be scene setup, animation or an override. Conversely a raw GI signal is not proof composition used it.

## 9. Performance contract

### Use the current submission completion point

`WorkloadQueue::threadRenderFrame` currently resets a two-entry timestamp pool, writes a start before `endFramebuffers`/`recordSetup`/framebuffers, writes end, then calls `execute(); wait();` and reads query results. This is **one workload graphics submission**, possibly one of several within an interpolated render occurrence. It is not presented-frame time, isolated RT time or CPU time.

Piggyback result collection on that existing wait. Add no waits, flushes or extra submissions. Do not implement a new asynchronous query framework now. If renderer scheduling later removes this wait, retire diagnostics with existing submission fences or add nonblocking completion support; never silently reintroduce a stall for telemetry.

Use these coarse disjoint intervals inside the existing whole-workload interval:

1. RT acceleration build block, including declared world-write/build dependencies and BLAS/TLAS barriers.
2. Fused RT tracing block, including its declared input/output transitions and binding.
3. Reconstruction block including its transitions.
4. Raster recording region per framebuffer, which includes lighting composition.
5. Optional diagnostic inset, only in diagnostic runs.

Adjacent boundaries must have documented ownership so barriers are neither omitted nor double-counted. Keep the whole-workload interval separately; report an untimed remainder. Whole + children is not an additive total. Start with combined BLAS/TLAS; split only if it changes an engineering decision. Shared setup/world-position processing outside these regions remains in the remainder until justified. Record framebuffer/workload multiplicity rather than averaging away repeated work.

`PrimaryRayGen` fuses primary, sunlight, AO/enclosure, local and GI work. Timestamps cannot measure these individually. Use feature ablations for **marginal workload cost**, including interactions and changed cache/branch behavior; do not label a subtraction an exact GI pass time. Do not split the production shader solely to profile it. The legacy `submitRaytracingScene`/denoiser/upscaler code is not this RT+ path.

### Existing Plume API constraints

Under `lib/rt64/src/contrib/plume`:

- `plume_render_interface.h`: `RenderQueryPool::queryResults()` has no success/availability return; no nonblocking fence poll is exposed by the current interface.
- `plume_vulkan.cpp::VulkanCommandList::writeTimestamp` uses bottom-of-pipe. `VulkanQueryPool::queryResults` reads **all entries**, no WAIT/availability flag, converts timestampPeriod to nanoseconds and logs errors without a valid result status.
- `plume_d3d12.cpp::D3D12CommandList::writeTimestamp` resolves one query immediately after each marker; extra markers have nonzero cost. `D3D12QueryPool::queryResults` converts by timestamp frequency.

For trustworthy export, make a small generic Plume addition: a success-returning timestamp result read with an explicit used range, leaving the existing interface available. Read only after the existing completion point. Validate written count/range and propagate backend failure as unavailable. Never export zero-filled/stale memory as a measured duration. A fallback implementation using an exact-sized pool still needs explicit failure handling. Allocate/reuse pools at arm/setup time; if active marker capacity is exceeded, skip the sample with a reason rather than allocate mid-measurement. Respect backend timestamp support/valid bits and convert units explicitly. Cross-backend absolute timing precision is not presumed equivalent.

CPU timings use monotonic steady-clock around classification/source preparation, RT resource preparation/allocation, command recording and the **already existing wait** separately. CPU submit/wait time is not shader time. Include allocation/resource-growth flags; no per-triangle timing. GPU timings cannot explain expensive CPU uploads or frame pacing alone.

### Benchmark procedure

- Use a reproducible held renderer workload for static cost, then a normal-motion sequence for streaming/HFR costs. The debugger's workload pause does not freeze simulation; record this distinction. Latch fixed interpolation for static runs.
- Fix resolution, settings, backend, mod stack, scene state and pacing. Warm shaders/resources; record/exclude initialization, resize, growth and skipped/interrupted runs from steady-state statistics. Keep their counts and optional separate startup measurements.
- Start with full stack vs reference, then GI off, locals off, AO/fill/shadows individually as needed. Preserve all other effective settings. GI-off can also remove reconstruction/shared queries; raw bypass cannot isolate reconstruction cost because it still dispatches.
- Report milliseconds, sample count, median/p95, elapsed window, exclusions and uncertainty/repeat spread. The reported 300 FPS versus100–130 FPS corresponds roughly to3.3 ms versus7.7–10 ms **presentation intervals**, not established GPU cost. Measure before assigning the difference.
- Compare instrumentation-off/on with otherwise identical production runs, alternating order and repeating. Do not subtract a guessed overhead. If overhead exceeds run-to-run variation or changes the conclusion, reduce marker frequency/count and report that the result is observer-sensitive. External GPU profiling is the escalation for unresolved fused-pass attribution; its own captures are labeled separately.

Do not use the existing rolling `ProfilingTimer::average()` as the benchmark dataset: its initially zero-filled history and missing per-sample correlation are unsuitable. Store a bounded valid sample array, serialize after measurement, and keep unsupported/failed/not-executed intervals distinct from zero duration.

## 10. Reproduction and scene survey

Store requested actions and observed state separately. The previous requested Field entrance produced an unconfirmed shooting-gallery-exterior view; capture labeling must use loaded `sceneId`, not the launch argument.

Generate a source-derived developer catalog later from the prepared report's known tables:

- absolute scenes from `include/tables/scene_table.h`;
- actual entrance/spawn/layer arrays in `src/code/z_scene_table.c`, resolving entries rather than enumerating all bit combinations;
- scene/layer entrance lists for spawn-to-room and room lists from headers;
- curated map-select destinations in `ovl_select/z_select.c::sScenes`.

Version the catalog against its input revision. Human-readable names are labels, not authority. Modded runtime observations may disagree with vanilla catalog predictions; preserve both and trust observed loaded state. Do not include ROM-derived assets in committed output.

Use existing normal entrance/pending-warp machinery and isolated save/config/input playback. Never directly set/request arbitrary room IDs. Observe normal room transitions: current room ID changes before DMA completion, old/new rooms can coexist, and commands can change lighting. For survey grouping, use `(sceneId, sceneLayer, settledRoom)` only when settled is justified. Record both room states always. The field predicate `status==0 && cur.segment!=NULL && prev.num==-1` is an **inferred settled candidate**, not a universal event; a `Room_FinishRoomChange` observation is stronger. Initially no new room hooks are required: do not auto-label uncertain transitions settled. Validate any future automated settled trigger on ordinary and special transitions.

A game snapshot is not a savestate and cannot recreate all quest flags, actor/effect state, camera scripts or mods. Reference the actual isolated checkpoint and deterministic input when available, retain run seed/time/settings, and verify observed state on replay. Avoid destructive save restores. Build an initial small regression set: known Town/local source, Inn authored-color, daylight, title/early cinematic motion, and a reported cutoff view. Expand based on failures; no manual room census in this task.

## 11. Diagnosis recipes and decision limits

| Symptom | Evidence sequence | Decision enabled / boundary |
|---|---|---|
| Dark or bright/flat scene | Verify game token and consumed environment; compare preset/adjustment/final inputs, Original/Native reference and Enhanced with features off; inspect receiver/ambient ownership, actual confidence/authority and clamp views; matched authority A/B | Separates original appearance, publication mismatch, fallback and Enhanced composition. Intentional art direction requires repeated correct-pipeline evidence plus artistic judgment, not a luminance threshold. |
| Missing apparent local | Context snapshot and draw-time bind attempts -> receipt/annotation -> value publication/cap -> selected surface and candidate -> finite influence/cosine/occlusion -> raster acceptance | Distinguishes no observed semantic source, binding coverage, adapter rejection, publication cap, receiver exclusion, selection cap and occlusion. Does not infer an emitter from appearance. |
| Characters lit, nearby world unlit | Compare bound owned slots versus unowned permission/selection, per-range traits and actual pixel acceptance | Different responsibilities may intentionally produce different response; no proximity-based ownership inference. |
| GI distant boundary | Crop raw validity, primary clipW/identity and traits across boundary; inspect primary TMax/projection/AS ranges; ledger bounce limits, misses, rejected hit traits, source selection and finite fade; then reconstruction confidence and raster acceptance | Separates receiver/primary coverage, bounded transport, submitted-scene absence, reconstruction and composition. A finite miss cannot prove what missing geometry would have done. |
| Motion/cinematic instability | Short consecutive occurrence burst: camera/matrices/weights, source inputs/order, raw/guides and reconstructed output; selected rays/seed/normal basis and confidence | Fixed pixel-stratified rotation has no time seed but moves relative to surfaces; source snapshots update at simulation rate. Compare raw change versus filter/authority change. No temporal history exists to blame and no stable object tracking is implied. |
| Low-poly faceting | Compare original shade, authored normal/magnitude and basis, geometric normal, owned direct/unowned direct, raw/reconstructed and ambient-transfer views on same geometry | Distinguishes authored normal data, intentional geometric spatial response, filter normal rejection and composition. Do not call geometric normal an authored smooth normal. |
| Sun transients | Published primary direction/RGB versus visual sun; actual bound direction/RGB match; camera/depth acceptance and visibility-query execution | Identifies semantic match/bridge/geometry/visibility boundary. No sunlight repair in instrumentation. |
| Full stack expensive | Clean workload GPU intervals + CPU preparation/wait + dimensions/multiplicity; controlled ablations | Assigns AS/reconstruction/raster/fused tracing or CPU/pacing cost. Internal fused feature costs remain marginal estimates. |
| Possible scene profile | Verify joins/coverage/fallback and generic limits across a few matched states first | A profile is a game-side future decision. Capture keys never become renderer scene-ID gates. |

A moving-pixel difference is not temporal surface variance: two screen pixels may depict different geometry. Initially use short sequences and manually selected coherent patches; do not compute “temporal GI error” without correspondences. Likewise screenshot brightness statistics alone are not energy conservation tests.

## 12. Output and validity contract

Default path: `_working-directory/diagnostics/lighting/<session>/<capture>/` (ignored).

Suggested files, emitted only when relevant:

- `manifest.json`: schema/build/units, capture mode, requested and actual occurrence, settings, artifact inventory/checksums, validation and drop/exclusion status.
- `game.json`: immutable phase snapshots and token/span associations.
- `renderer.json`: consumed metadata, effective params, call/range/source tables and aggregate counts.
- `bindings.json`: capped bind/receipt/annotation/lineage records for selected capture.
- `picks.json`: ray records and reconstruction explanation with observation versus recomputation labels.
- `timings.json`: raw bounded valid samples plus summaries and exclusions.
- `*.bin` + descriptor entries: native-format buffer crops; `*.png`: beauty and labeled previews.
- `investigation.md`: human hypothesis, supporting artifact/pick IDs, alternative explanations, decision and remaining gaps. Not machine-measured truth.

JSON is a transport choice, not a requirement to serialize game structs wholesale. Every measurement has a defined scope, unit, producer and validity. Use explicit statuses such as `observed`, `not_executed`, `unsupported`, `unassociated`, `truncated`, `failed`, `recomputed`; absent data is null with reason, never a meaningful numeric0. GPU float-bit containers and nonfinite values are encoded losslessly in binary/hex, not invalid JSON. Enumerations carry raw numeric value plus versioned names. Export actual source record floats, not rounded UI values. A count with truncation is a lower bound only for its defined observed population.

Minimum manifest example (illustrative field names):

```json
{
  "schema_version": 1,
  "mode": "snapshot",
  "session": "uuid",
  "render_occurrence": 42,
  "workload_id": 18,
  "framebuffer_pair": 0,
  "game_association": {"status": "observed", "token": 11, "phase": "pre_draw"},
  "gpu_timing": {"status": "not_executed", "reason": "diagnostic_shader"},
  "artifacts": []
}
```

The implementation adds required build/configuration/artifact details; the example does not license omitting them. Schema readers reject unsupported major versions and preserve unknown optional fields. Capture failure must never change fallback decisions or source eligibility.

## 13. Implementation order and acceptance gates

Implement as independently reviewable steps, preserving Native and current rendering equations:

1. **Correlation + capture envelope.** Developer token, pre/post draw snapshot copies, task-finalization lifetime, RT64 annotation spans, Workload/render/config identity and JSON writer. Test delayed consumption, reused list memory, repeated lists, OPA/XLU, multiple workloads, HFR, missing marker, title/non-Play and capacity exhaustion. Missing joins must fail closed as unavailable. This is the first gate; no trustworthy scene-labeled GPU export before it passes.
2. **CPU source/receiver evidence.** Observe receipt outcomes and command lineage; preserve production equivalence. Exercise verified, consumed, modified, unbound, cap/dummy, equal-valued sources and unassociated mod annotations. Check aggregates reconcile with rows and disabled diagnostics do not change submitted light/range data.
3. **Clean performance capture.** Generic success/range timestamp API, bounded coarse intervals at existing completion, valid sample export. Verify unused queries, disabled passes, backend failure, timing-disabled overhead and GPU units. Validate Vulkan on current hardware; build D3D12 and qualify runtime separately. Do not claim unsupported APIs tested.
4. **Existing-buffer snapshot + views.** Select exact target/occurrence, preserve native formats/strides and resource lifetime. Validate render-to-crop coordinate mapping and that displayed preview/pick references the captured occurrence. Add visual composition/normal/validity views incrementally without varying ABI changes.
5. **Selected ray ledger and short burst.** Add only after an existing-buffer case demonstrates ambiguity, or as the planned required tool for cutoff/motion investigations. Single-writer bounded records; compare diagnostic and production raw/guides at fixed inputs within storage precision. Check reconstructed output against tap explanation, accepted/invalid/miss outcomes and actual source candidates. Mark all these runs non-benchmark.
6. **Survey conveniences.** Generate catalog, add capture recipes and indexing of existing capture directories after a few real diagnoses. No automatic art-profile generation or exhaustive scene runner required.

Meaningful runtime acceptance includes one settled scene and room transition, one active local with a receiver and a fallback draw, one fixed-input repeat, one HFR/motion burst and one clean timing window. Preserve representative failure artifacts. Focused tests should exercise correlation/lifetime, packing and validity, not mirror all renderer math. Shader edits require the documented real SPIR-V/DXIL build route; visual views require runtime checks, not compile-only signoff.

### Work performed in this architecture run

No renderer or instrumentation code was implemented. The contract-critical facts were decidable from targeted source inspection: current buffer semantics, source association loss, fused tracing, task scheduling and timestamp limitations. A partial prototype would not validate the missing end-to-end correlation or both query backends and would add an unqualified implementation to the handoff.

The work can be implemented by a Sol-class agent in the stages above. It must follow the explicit semantics and gates rather than inventing metric names from existing debug colors. Marker placement/lifetime and GPU query failure handling are correctness work, not clerical changes, but their choices are now bounded and reviewable. Stop the affected feature at unavailable if a gate fails; unaffected evidence can ship independently.

### Remaining limitations

No runtime instrumentation validation is claimed in this run. The exact new marker encoding/placement, native snapshot ABI and timestamp API additions still require implementation qualification. Generic light owner and full scene geometry remain unknowable from these inputs; no investigation is needed to pretend otherwise. Special-room settled behavior, mods bypassing observation, raster overdraw/MSAA numeric attribution, D3D12/HFR coverage and observer overhead remain explicitly bounded validation areas. None permits silently assigning a cause from missing evidence.
