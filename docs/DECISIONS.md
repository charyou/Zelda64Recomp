# Architecture Decisions

> Durable architectural decisions for this project.
> Add an entry only when a choice materially constrains or guides future work.
> Do not use this file as a session log; current implementation state belongs in `HANDOFF.md`.

## ADR-001 — Native rendering is the compatibility reference

**Status:** Accepted

**Context:** Modern per-pixel effects can alter data observed by framebuffer feedback, reinterpretation, and other game-visible N64 behavior.

**Decision:** Keep Original vertex fog and other original behavior in the Native/RDRAM renderer. Apply modern fog only during enhanced Workload replay.

**Why:** This preserves game-visible rendering behavior while allowing high-resolution beauty rendering to improve spatial fidelity.

**Alternatives considered:** Applying the feature to both renderers; gating by output resolution. Both are less explicit and risk changing compatibility behavior.

**Consequences:** New beauty-render features need an explicit enhanced-renderer gate and must not rely on Native output changing with them.

## ADR-002 — MM environment semantics are Workload metadata

**Status:** Accepted

**Context:** Atmospheric rendering needs resolved MM environment state that is lost when `Play_SetFog` quantizes it to RSP coefficients.

**Decision:** Zelda64Recomp extracts resolved `LightContext` fog color, fogNear, and zFar plus the resolved environment sun vector, active world-camera state, conservative outdoor classification, current precipitation, storm state, and nearby active collision-water coverage, then passes them through the native renderer adapter. RT64 snapshots the generic atmosphere structure onto the Workload before display-list processing. Water coverage contributes to the generic wet-air influence; automatic skyless views receive conservative distance attenuation. Explicit mod overrides retain priority.

**Why:** A Workload is the stable per-frame render record and avoids render-thread/HFR races. MM logic remains in the game integration layer; RT64 receives only generic resolved inputs.

**Alternatives considered:** Extended GBI commands for frame-global values; render-thread mutable globals; recreating time/weather logic in RT64.

**Consequences:** Extended GBI remains reserved for draw-/transform-local information. Future frame-global semantic inputs should extend Workload metadata unless they truly vary within a display list.

## ADR-003 — Atmospheric fog replaces only the classified environment baseline

**Status:** Accepted for the experimental first implementation

**Context:** MM frequently overrides fog for actors and effects, then restores `Play_SetFog`. Applying one atmosphere model to every fogged draw would erase those local choices or double-fog them.

**Decision:** Compare each draw's final RSP fog coefficients and fog RGB with the signature derived from the resolved environment state. Atmospheric eligibility additionally requires a perspective projection whose inferred camera position and orientation match MM's active world camera. Matching world draws may use Atmospheric fog, with a semantically and optically headroom-gated share of the authored optical depth redistributed into a height-dependent medium rather than stacked as a second fog curve. Classified outdoor views may also use a clear-/wet-air transmittance floor combined by maximum optical depth, never addition. Automatic skyless views use a conservative distance-attenuated floor; explicit mod OFF remains zero. Nonmatching local/effect and secondary-camera/UI draws use Faithful Per-Pixel fog; mixed per-vertex-state draws use Original.

**Why:** It preserves per-draw authoring and existing N64 blender semantics without requiring MM-specific scene profiles inside RT64.

**Alternatives considered:** Frame-global replacement of every fog draw; fullscreen depth fog; adding atmosphere after legacy fog. These lose local semantics, mishandle transparency, or visibly double fog.

**Consequences:** Signature derivation must remain behaviorally aligned with MM's `Play_SetFog`/`Gfx_SetFogWithSync` conversion. Camera matching must remain semantic rather than scene-ID-specific so pause models and other secondary projections cannot interpret their coordinates as world-space atmosphere. Runtime captures should validate classification before the mode is presented as non-experimental.

## ADR-004 — Preserve RT64's raster-stage linkage ABI for modern fog

**Status:** Accepted

**Context:** Passing clip-space fog depth through a new `TEXCOORD1` varying produced severe color and geometry corruption in Vulkan and a D3D12 driver crash on an AMD RDNA4 GPU. An in-game A/B build with the added varying removed rendered correctly in all three fog modes while retaining the extended RDP parameter layout.

**Decision:** Do not add a raster-stage varying for atmospheric fog depth. Reconstruct clip W in the pixel shader from reciprocal `SV_Position.w`; keep RT64's established vertex/pixel linkage signature unchanged.

**Why:** The reconstruction supplies the required per-fragment distance input without expanding the linked library or specialized shader ABI. It also preserves the compatibility-tested attribute locations across Vulkan and D3D12.

**Alternatives considered:** Keeping the extra varying; disabling modern fog on RDNA4; adding a separate shader permutation. The first is demonstrably broken on current hardware, the second loses the feature, and the third increases shader-cache and linkage complexity unnecessarily.

**Consequences:** Future raster enhancements should prefer values already available from system semantics or existing interpolants. Any new cross-stage varying requires in-game validation on both Vulkan and D3D12, including AMD hardware, before acceptance.

**Amendment 2026-09-24:** Pixel-shader `SV_Position.w` is backend-specific. It is 1/w in Vulkan SPIR-V (FragCoord) and clip w in D3D DXIL; DXC `-fvk-use-dx-position-w` exists for exactly this. Reconstruct clip W per target (`#ifdef __spirv__`). The RT receiver gate did so from 2026-09-24. The fog path reads only `SV_Position.z`.

## ADR-005 — Atmospheric art direction is a masked per-frame mod event

**Status:** Accepted for the experimental Atmospheric mode

**Context:** A global preset cannot cover deliberately stronger regions such as Southern Swamp, unusual rooms, or cutscenes whose skybox is temporarily hidden. Scene tables and room IDs are Majora-specific and do not belong in generic RT64 code.

**Decision:** Zelda64Recomp publishes `recomp_on_atmosphere_override` once per gameplay frame with initialized defaults and an explicit field mask. Code mods may select scenes, rooms, cutscene state, time, or weather through `PlayState` and override only the atmospheric fields they own. Overrides are copied into Workload metadata and affect only enhanced Atmospheric replay. A separate automatic signal classifies natural-sky normal rooms as outdoor even when the skybox draw is disabled; F1 can A/B this expansion, while a masked mod value takes precedence.

**Why:** Per-frame masked ownership prevents state leakage, permits small composable area profiles, and keeps MM policy out of RT64. It also retains conservative defaults for genuinely indoor shops while allowing explicit art direction where useful.

**Consequences:** The event ABI and public struct layout must remain versioned compatibly once released. Multiple mods claiming the same field resolve by normal callback order. Original/Native and MM's `LightContext` must never consume these values.

## ADR-006 — Withdraw camera-basis publication; preserve affine model transforms

**Status:** Original experiment withdrawn; replacement boundary accepted

**Context:** The experiment published MM's main camera through `gEXSetViewMatrixFloat` and `gEXSetProjMatrixFloat` while preserving the fixed display-list commands. Default-off switches substituted fixed camera matrices into those Extended GBI commands. This still changed RT64's extended coordinate basis: `RSP::setVertexCommon` multiplied that camera view-projection into every model/world transform. The assumption that publication was neutral with both switches off was false.

**Decision:** Remove Zelda's added `View_ApplyPerspective` patch, parallel float generation, unconditional camera-basis publication, and precision switches. The existing camera interpolation tags continue to use MM's original fixed perspective/distortion/view path. Restore RT64's upstream float-command handlers so mods retain the documented generic commands without fork-wide overrides. Keep exact fixed RSP camera references for semantic fog classification, aligned with the other projection arrays including their identity sentinel.

**Evidence:** A fresh shader build with the semantic-camera sentinel correction still produced radial geometry wedges at the Southern Swamp owl. Omitting only the four camera-publication commands, with synchronized patch metadata, restored a clean view at the same checkpoint and mod profile. This establishes publication as causal for that reproduced failure; it does not qualify every scene or individually attribute the failure to one downstream matrix operation.

**Why:** Source inspection identifies two incompatible downstream assumptions. Model decomposition normalizes by matrix `[3][3]` without retaining the homogeneous factor; baking projection into model transforms introduces near-zero or negative factors and changes model-space lighting/interpolation inputs. Separately, enhanced projection processing recomposes `inverse(EV) * V * inverse(EP) * P`, which generally differs from the exact full correction `inverse(EV * EP) * (V * P)`. Disabling interpolation alone cannot guarantee neutral output.

**Consequences:** Future camera precision work must preserve affine model/world transforms, preserve Native/RDRAM inputs, carry camera precision through an explicit camera/projection representation, and account for secondary views, state lifetime, and MM's resolved `View_StepDistortion`. Generic Extended GBI support remains available; it is not assumed to be a drop-in camera replacement. Qualification must include ordinary play, modded content, skyboxes, pause/UI cameras, cutscenes, distortion, lighting, and high frame rates before enabling a replacement.

## ADR-007 — Per-pixel diffuse lighting reuses actual RSP semantics

**Status:** Accepted; coverage refinement 2026-09-09

**Context:** RT64 already retains authored normals, ambient/directional lights, per-vertex lighting state and interpolated world transforms. Reconstructing MM lighting in a second semantic system is unnecessary to improve coarse vertex-lit characters.

**Decision:** Evaluate existing lights per pixel on eligible enhanced draws. Compare light values rather than buffer identity. Carry normal direction through smooth RGB and authored magnitude through a scalar interpolant; zero/short/varying normals are supported without a draw-wide length gate. A shared world matrix uses the original local directional-light equation, including nonuniform scale/shear. Different skeleton matrices use a common rotated basis when their transforms are compatible. Preserve original vertex lighting in Native and on unsupported draws, including actual positional microcode lights and modified colors. No Zelda identities or assumptions about vanilla meshes belong in this renderer feature.

**Why:** Visible Town diagnostics showed that the initial draw-wide normal-length gate rejected large surfaces. Per-vertex magnitude transport preserves the shading information that motivated the gate and removes its coarse fallback without batch splitting. Draw-local alpha/fog and combiner/blender behavior remain intact. HFR uses the renderer's already interpolated matrices. The added scalar varying passed the current Vulkan runtime check; this does not qualify D3D12 runtime or supersede atmospheric camera constraints.

**Consequences:** Lighting is independently selectable from fog. Future positional lighting or shadows must preserve the current fallback rather than silently dropping unsupported lights. New vertex system inputs and RDP layout changes still require synchronized shader builds and runtime API checks. See `docs/PER_PIXEL_LIGHTING.md` for eligibility and integration details.

## ADR-008 — Hardware RT starts with isolated primary-hit visibility

**Status:** Accepted for the experimental Vulkan developer path, 2026-09-09

**Decision:** RT64 owns a minimal per-framebuffer RT resource owner using Plume's existing BLAS/TLAS, pipeline, SBT and trace APIs. Rebuild a world-space BLAS from conservative executable opaque indexed ranges and the existing presentation-time world-position buffer, then use an identity TLAS instance. The first consumer is a visible primary-hit inset with a separate descriptor/push-constant ABI. Preserve all normal raster draws and Native behavior; do not enable the incomplete legacy `RT_ENABLED` renderer or change raster-stage linkage to establish this foundation.

**Why:** This delivered observable hardware intersections on real Town/Link geometry without restoring historical DI/GI/denoiser classes or touching the fragile lighting/fog ABI. Backend AS correctness repairs belong in the nested Plume source. The initial Vulkan multi-geometry range bug demonstrates that surviving API declarations alone are not runtime proof.

**Consequences:** This diagnostic's double-sided, single-projection, opaque subset is not a production shadow contract. Alpha, clipping, culling, receiver semantics and caching remain separate work. Keep the primary-hit view as a regression reference for the next visibility consumer. Vulkan runtime is confirmed; D3D12, MSAA and broad HFR/scene-transition behavior remain unqualified. See `docs/RAYTRACING_FOUNDATION.md` for the exact implementation and evidence.


## ADR-009 — Spatial RT refines bounded authored fill through shared signals

Status: accepted, 2026-09-11.

Run 3 uses the same submitted opaque scene, SurfaceHit and finite nearest-hit queries for contact and environment enclosure. Raw contact/environment visibility and geometric normals remain separate from hit identity/depth and from the ambient response. The resource contract is generic and backend-neutral; future reconstruction may consume it without owning MM semantics. No temporal identity or complete sky coverage is implied.

On compatible RSP-lit geometry, only the explicit authored ambient term changes; direct lighting keeps its existing ownership. On conventional opaque texture-times-SHADE content without a recoverable ambient term, Enhanced may instead reserve a configurable authored-fill budget, bounded by both SHADE RGB and resolved environment ambient. This is an artistic partition of existing appearance, not a physical decomposition or proof of non-emission. Unsupported/special combiners and Native retain their original paths. Combined contact/enclosure retains one protected floor to avoid compounded blackening; legacy actor shadows remain.

Zelda64Recomp publishes post-adjustment ambient RGB and a broad hemisphere-lobe hint through the existing per-frame environment bridge. RT64 receives resolved generic values, not MM IDs or lighting-mode policy. Natural-sky presentation metadata shapes the lobe; finite TLAS queries describe nearby obstruction only. A miss never establishes open sky. Skyless rooms use isotropic authored fill, and absent geometry may under-occlude it. This bounded approximation is intentionally not GI or physical skylight transport.

Controls and tuning use the existing runtime/configuration path. AO, Environment Fill and Sun Shadows are independent; numerical budgets/ranges/sample count/bias are shared settings available to F1 and Graphics. Future quality presets should reuse those settings, not introduce another owner.

## ADR-010 — Semantic local sources replace verified RSP contributions

Status: accepted, 2026-09-13.

Generic source response is independent of game identity and visible-emitter appearance. The first production consumer attaches a source record to a verified original RSP slot, invalidates it on ordinary light reload/color edits, and snapshots it at vertex load. MM owns verification of its positional/reference-directional binding realizations; RT64 owns per-pixel response and finite source visibility. Only accepted owned terms are replaced. Tagged positional draws retain original interpolated SHADE when RT receiver validation fails. Other directional and ambient responsibilities remain separate.

The initial path enhances bound sources on supported characters, static props and positional-lit world geometry. Unbound source application is not inferred from proximity or bright textures. Future game profiles may explicitly permit spatial or synthetic treatment using the same generic source/visibility functions, with independent receiver permission and source collection. No second renderer is required, and no missing-semantic case implicitly authorizes synthetic lighting. See [SEMANTIC_LOCAL_LIGHTS.md](SEMANTIC_LOCAL_LIGHTS.md) for ABI, response, adapter lifetime and limitations.

## ADR-011 — Spatial receivers and replaceable indirect reconstruction

Status: accepted, 2026-09-14. Extends ADR-009 artistic fill authority; preserves ADR-010 owned replacement.

RT64 classifies safe spatial receivers from executable opaque world-camera geometry, original vertex provenance and monotonic SHADE response. Original-light eligibility, receiver capabilities, source permission, direct ownership, visibility policy and indirect participation are independent. Verified source records may authorize bounded response on unowned receivers; they never remove unverified original contributions. MM publishes resolved primary/secondary environment sources and profile permissions through its existing bridge. RT64 contains no game IDs or emitter guesses.

GI uses one finite diffuse bounce, conservative authored appearance tint and at most two relevant verified local contributors selected once per primary receiver. Raw indirect and renderer-owned depth/normal/validity guides feed a separate reconstruction backend. The initial spatial backend has no temporal history. Vendor reconstruction is a future adapter, not a reason to reshape source/surface semantics.

RT+ transfers a confidence-weighted share of supported ambient/fill to reconstructed transport plus finite enclosure/contact. Authored-lit ambient is explicit; authored-color content uses a declared artistic partition. Primary directional visibility and secondary direct remain separate, and Run-4 locals retain exact replacement. Invalid environment/receiver/guide responsibilities preserve their existing contribution. Neighbor support confidence and common monotonic SHADE coverage reduce mixed-coverage discontinuity without game-specific grouping. See SPATIAL_LIGHTING.md for equations, budgets and current limits.

## ADR-012 — Primary environment RGB is an Enhanced energy reference

Status: accepted, 2026-09-22. Extends ADR-007/011 only for primary environment direct.

Preserve semantic source direction, hue, relative environment strength and zero energy; allow Enhanced to reinterpret absolute direct magnitude. Direct authority follows global RT+ authority, with a session override. A shared raster/GI gain `1 + authority * smoothstep(0.08,0.65,max(RGB))` stays within 1–2x, leaves very weak sources unchanged and never derives daylight from a valid direction alone. No celestial/elevation policy or new game-specific semantics are required.

Raster transfers one uniquely matched historical primary contribution, rather than adding another source. Source interpretation does not depend on having an RT guide; visibility remains geometric obstruction of only that direct term. GI uses the same interpreted source once before its own transport bounds. Existing ambient/fill ownership and final stylized SHADE saturation remain independent. Unknown/ambiguous ownership retains authored fallback. Retaining current raster receiver eligibility is a bounded safety decision, not a principle that historical light slots define all future modern source influence.

Reject normalized fixed-strength sunlight, authored RGB as an absolute Enhanced ceiling, and camera-dependent mean-visibility energy compensation. The first loses weak/weather intent; the second leaves Enhanced primarily subtractive; the third couples lighting energy to framing and incomplete caster submission. This decision does not introduce physical lighting, exposure, extra rays or global framebuffer correction.

Default interpretation is enabled within Enhanced after the held daylight/weak-night Vulkan checks; authored magnitude remains available through Direct authority zero or interpretation off. See [Primary environment direct](PRIMARY_ENVIRONMENT_DIRECT.md) for exact ownership, configuration, ABI, evidence and remaining limits. Native, secondary direct, local ownership, camera/culling/cadence and reconstruction responsibilities are unchanged.

## ADR-013 — Source permission and historical decomposition are distinct

Status: accepted after focused Vulkan qualification, 2026-09-22. Extends ADR-011/012.

Historical ownership authorizes removal/replacement of a known contribution; it does not set a permanent modern receiver ceiling. Modern influence separately requires trustworthy source semantics, explicit source permission and supported receiver semantics. Reuse existing spatial traits and source permission conventions rather than introducing a universal lighting object.

Primary keeps ADR-012 unique-slot replacement. On identity/depth-validated spatial receivers with no Primary match, a profile may authorize bounded additional response from the same source's gain-minus-one. Preserve all uncertain authored contributions. Scalar bounds and appearance headroom preserve source hue without treating SHADE as albedo or claiming any historical decomposition. Multiple matching contributions and positional fallback remain conservative. Authority scales the additional responsibility continuously; permission itself is not a fractional semantic guess. Visibility obstructs only the modern Primary response and reuses the existing directional query.

The generic primary color record's previously unused W grants unowned response; omission defaults to zero. MM publishes .35 in its adapter. Raster caps the increment at .15 peak before scalar remaining-SHADE headroom. This is an explicit stylized additional responsibility, not full modern light laid over uncertain old lighting. Reject both wholesale SHADE subtraction and full unowned source addition. No change to ADR-012 energy interpretation or GI transport.

Semantic Locals already meet this distinction through response.w, receiver traits, exact owned replacement and bound-source exclusion; retain their equations and controls. Secondary has resolved source data for existing GI but no published unowned raster permission or incremental energy contract; do not invent one by copying Primary's gain. Ambient remains a confidence-weighted exact/artistic fill transfer, independent of directional visibility and slot matching. These responsibilities share the principle and fallback discipline, not identical shader math.

Session Primary-expansion control restores ADR-012 alone. Primary interpretation off/Direct authority zero disables the added responsibility completely. F9 applies one effective master override above configured RT+ features; it never rewrites configuration or developer overrides. Ordinary raster MSAA, resolution, presentation and mod/texture replacements retain their configured baseline behavior. MSAA sample changes require application-wide shader-cache/target reconstruction and are not a per-frame RT+ responsibility; enhanced cutout coverage does return to its original path.

## ADR-014 — Environment directionals carry geometric visibility authority

Status: accepted after focused Vulkan qualification, 2026-09-24. Refines ADR-012/013; does not change their equations for geometric sources.

A valid direction does not establish that a directional is a geometric light. The adapter publishes, per environment directional, a visibility authority in [0,1] (`AtmosphereParameters::environmentDirection[i].w`; zero denies). RT64 treats it generically. It is the source's geometric role, and it scales every modern responsibility that depends on that role:

- owned-term shadowing uses `lerp(1, traced, authority)`; authority 0 traces no primary rays;
- ADR-012 energy reinterpretation uses `directAuthority * authority`, so a non-geometric fill keeps its authored magnitude;
- ADR-013 unowned additions require traced geometric visibility: `increment * traced * authority`. Missing visibility means no addition, never an unoccluded one. Expansion requests primary rays itself (spatial bit 64) even when raster sun shadows are off;
- GI bounce transport from the primary is weighted by authority.

MM derives authority from its own directional-shadow rule. `ActorShadow_DrawFeet` lets a directional cast only while `dir.y > 0`, weighted by RGB·|y|. The adapter uses `smoothstep(0, 0.1, normalized y)`; RGB weighting stays in the existing energy terms. In time mode `dirLight1` points below the horizon at night: it is authored fill, not a shadow caster.

Rejected alternatives:
- an elevation branch in RT64: that is game policy;
- tracing and then gating by energy: that spends rays to remove authored fill;
- keeping amplification for non-geometric fills: once no longer masked by false occlusion it over-brightened night undersides.

The secondary also receives published authority. RT64 does not yet realize secondary visibility; that is an open responsibility, not an implied permission.

`FramebufferParams` offset 32 is now `primaryVisibility` (x traced, y authority, w owned-term application). Its former `shadowSun.xyz` had no GPU reader. Layout sizes are unchanged. See PRIMARY_ENVIRONMENT_DIRECT.md.

## ADR-015 — RT scene membership stays submission-defined; completeness belongs to the submitter

Status: accepted, 2026-09-24. Refines ADR-008/011 scene boundary.

Camera-motion evidence showed what actually changes (RAYTRACING_FOUNDATION.md, 2026-09-24 section):
- The largest loss was renderer-side: primary guides spanned D3D-style NDC 0..0.99 on GL-style N64 clip space. Receivers closer than about 2·zNear lost RT (up to 43% of frames near walls). Receivers beyond the far window were lost too (1.4% of Termina Field hits).
- Remaining population churn is game culling. Actor draw culling (tagged transform groups entering/leaving) dominates. Behind-camera cullable-room entries are a small share.
- World-camera agreement never failed, and HFR occurrences carry identical scenes.

Decisions:
- Primary rays span the full rasterized depth range (NDC −1..1).
- RT64 does not persist or synthesize geometry the game did not submit this Workload.
- Where culling is a historical performance realization with no game-visible effect, the adapter may submit the culled opaque geometry. MM does this for cullable-room entries entirely behind the camera: zero raster pixels, same order, beyond-zFar and XLU unchanged.
- Actor draw culling is coupled to actor draw side effects and is not bypassed.
- Stock transform-group IDs are the stable identity that any future persistence or temporal policy must key on. Untagged draws use frame-pool matrices and have no cross-frame identity.
- Scene lifetime work (reuse per Workload, refit per HFR occurrence) is performance and temporal groundwork, not a shadow-completeness fix.

**Amendment 2026-09-25 (ADR-016):** lifetime is implemented as one BLAS per game frame refit across its HFR occurrences. Untagged geometry still has no transform identity; exactly unchanged untagged geometry gets content identity, and nothing is retained beyond submission.

## ADR-016 — The RT scene is a renderer-owned record with explicit lifetime, identity and requirements

Status: accepted after Vulkan qualification, 2026-09-25. Implements the lifetime part of ADR-015; membership is unchanged.

Owner and lifetime:
- Each framebuffer slot owns one `RaytracingSceneRecord`, shared by every RT+ consumer of that framebuffer. There is still no scene spanning framebuffers or projections; that would change membership.
- Surfaces are exactly this occurrence's submitted executable opaque ranges. GeometryIndex stays the occurrence-local surface index.
- The game frame is the Workload submission. Continuity is per logical framebuffer: depth image, width, format and target size. Color images rotate through RDRAM every frame, so their address is excluded.

Bottom-level structure:
- One BLAS is fully built once per game frame, with updates allowed.
- Later HFR occurrences of the same Workload refit it in place. Refit requires the same Workload, the same world-position and index buffers, and the same ordered index ranges, so index data is identical and only interpolated positions change.
- When every surface is occurrence-invariant and the ordered exact content equals what the structure holds, the structure is reused without any build.
- Anything else gets a full build. The TLAS (one identity instance) is rebuilt whenever the BLAS changes.
- Plume gained a generic `updateBottomLevelAS` plus `allowUpdate`/`updateScratchSize` build info (Vulkan and D3D12; Metal RT remains unimplemented). No RT64 or MM policy lives in Plume.

Surface provenance (CPU-side, backend-neutral; no GPU ABI change):
- Per-transform matching provenance comes from the stock TransformProcessor: unassociated, matched unchanged (bitwise), or matched moved.
- Keys:
  - `topologyKey` is connectivity: relative indices and per-vertex transform slots;
  - `shapeKey` is model-space positions;
  - `contentKey` combines topology, shape, vertex velocity and raw world matrices, i.e. the exact world-space inputs.
- Identity:
  - `Tagged` comes from explicit gEXMatrixGroup IDs of the referenced transforms, the stock call-matching material key (combiner, other mode, geometry mode) and the submission ordinal within that key. Draw calls under one limb commonly differ only by texture state.
  - `Content` is exact unchanged world-space inputs of untagged, occurrence-invariant geometry.
  - `None` covers everything else. Continuity is New, Continuous, TopologyChanged or Ambiguous.
- Motion:
  - `Static` means exact content equality with the previous game frame;
  - `Interpolated` means a matched Tagged identity with continuous topology and either unchanged shape or explicit vertex velocity;
  - `Unknown` covers the rest. Stock `worldVelBuffer` (per display occurrence) is the canonical motion signal only for Static and Interpolated surfaces.
- Heuristic AUTO matching, ambiguity, new geometry and unvelocitized deformation never produce trusted identity or motion. They still render through the conservative current-frame path.

Requirements:
- `RaytracingRequirements` names every consumer: insets, owned shadows, primary expansion, AO, environment fill, locals, spatial locals, GI, raw GI and diagnostic view. It derives scene need, guide need and the unchanged trace-mode ABI.
- Stock `raytracingEnabled` again means only the legacy RT_ENABLED renderer.
- `WorkloadConfiguration::rtPlusPrerequisites` states the genuine prerequisite: RT+ traces stock VertexProcessor world positions, whose inputs exist only for matched frames. It keeps matching and world-vertex processing active while any RT+ consumer is configured.
- With no consumer, or with the master override off, there is no scene work.

Rejected, with evidence:
- A static/dynamic two-BLAS split. In Town, about 70% of RT triangles are animated tagged actors. AS build fell only from 0.40 to 0.30 ms, while two overlapping instances raised trace cost 24% at product resolution (1.18 to 1.47 ms). It was also slower at 20 Hz.
- Per-object BLAS with TLAS transforms. N64 multi-matrix skinned triangles and non-affine matrices prevent it being universal, it needs an allocator, and it has no measured need.
- A scene spanning framebuffers.
- Refit across game frames. Index data is not guaranteed identical, and BVH quality would degrade without bound.
- Using the color address in continuity.

Consequences:
- ~~Future temporal consumers key history on Tagged/Content identity with explicit continuity, and use stock motion only for trusted motion classes.~~ Superseded by the amendment below and ADR-017.
- A future derived caster or proxy, or an explicitly authorized finite emitter, can reference a surface's identity and topology/shape keys without replacing this lifetime.
- Refit trace quality is bounded to one game frame of interpolation. Measured trace cost is unchanged.

**Amendment 2026-09-25 (identity is candidate lineage, not correspondence):** `rt64_wp3_identity_fixture` runs the production `describe`/`finalize` on equal-input Tagged surfaces. A,B → B,A, insertion and removal keep an ordinal's key and report `Continuous` for the wrong logical surface. With explicit vertex velocity, that wrong lineage is also `Interpolated` ([validation](reviews/RT_PLUS_PRE_NEXT_WP_VALIDATION_2026-09-25.md)). The lifetime/refit/reuse decisions are unaffected, because they key on exact content and index data, not on identity.

The provenance contract is therefore narrowed. The implementation is unchanged, and the fixture stays as regression evidence of the limit.
- Identity and continuity are **candidate lineage**: statistics, and a possible input to future keyed caches. They never establish that a history sample belongs to the same logical surface.
- `Static` / `Interpolated` mean only that the stock per-vertex motion (`worldVelBuffer`) is **admissible** for the surface: exact unchanged content, or matched transforms without unvelocitized deformation relative to the candidate lineage. `Unknown` means it is not admissible.
- Admissible motion is still only a hypothesis about where to look in the previous occurrence. The motion vector itself comes from the stock transform association and the vertex's own velocity, not from the ordinal lineage. The fixture's false lineage therefore does not falsify the vector. It does disqualify any consumer that skips independent validation.
- Every history consumer must establish correspondence itself (ADR-017). Do not "fix" the fixture by extending the identity key toward a persistent object-identity system.

## ADR-017 — Temporal history validity belongs to the consumer: geometric correspondence in the stored previous occurrence

Status: accepted, 2026-09-25. Implemented and Vulkan-qualified for Clock Town at Original refresh (images) and 144 Hz (occurrence series). Details: [INDIRECT_RECONSTRUCTION.md](INDIRECT_RECONSTRUCTION.md). Extends ADR-011 reconstruction; relies on the ADR-016 amendment.

First consumer: bounded diffuse GI. Its raw signal is incident indirect irradiance at the primary receiver: the sum over bounce hits of incident light times the bounded tint of the *hit* surface. Receiver response is applied only in composition. The signal is therefore a function of world position, orientation and scene state, not of which logical surface is the receiver. It is demodulated by construction.

Correspondence evidence. A history sample may contribute to a pixel only if all of the following hold. Otherwise the pixel restarts from the history-free result.
- **Occurrence continuity:** the history was written by the immediately preceding RT occurrence of this framebuffer slot. That means the same framebuffer key, size and consumer configuration, scene framebuffer continuity, and no failure, reset or resource recreation.
- **Current validity:** the current signal and receiver guides are valid.
- **Admissible motion** (ADR-016 amendment). `Interpolated` also requires that stock motion can be aligned to the stored occurrence (below).
- **In bounds:** the reprojected position lies inside the target.
- **Geometric agreement:** the stored previous guides at that position agree, per bilinear tap, with the predicted previous linear depth (relative tolerance) and with the normal.
- **Stored validity:** the stored history sample is valid.

Surface identity equality is never evidence. Identity inequality is not used either, because irradiance history is location-based and lineage keys would only add false rejections. A future receiver-dependent signal (specular, non-demodulated radiance) must add its own receiver-consistency evidence. It must not reuse candidate lineage for that purpose.

Occurrence alignment:
- **Camera:** reprojection uses the view-projection stored with the history occurrence. This is exact, including across skipped HFR ticks.
- **Stock object motion:** spans `prevFrameWeight → curFrameWeight`.
  - Within one Workload, it is rescaled to the stored occurrence's weight. This is exact under linear interpolation.
  - Across the adjacent Workload, it is extended at constant velocity. This is inexact: normally it covers one display tick, it is limited to half a game frame (heavy frame skipping is refused), and the geometric test guards it.
  - Anything else makes `Interpolated` inadmissible.

Radiometric change. There is no explicit light or source invalidation; the stage only bounds responsiveness, and establishes no radiometric validity. Implementation evidence refined the first design:
- History length is bounded in *game frames* (6), not occurrences, because game state changes per Workload. The occurrence cap is 15.
- History ages with normal change: irradiance depends on orientation, so a rotating surface keeps correspondence but not its irradiance.
- The clamp uses raw-signal neighbourhood statistics. Spatial-result statistics collapse onto the current noisy estimate, which discarded accumulation and darkened converged history by 3.5%.
- A moving actor's indirect-shadow lag on nearby receivers remains a measured, bounded limit.

Sampling. The temporal backend requests a per-occurrence sequence index, and the GI hemisphere rotation advances with it. Without history, the fixed rotation remains: a varying seed without accumulation only adds shimmer.

Boundary. The canonical per-occurrence inputs are:
- raw signal: RGB, plus hit distance or −1;
- linear depth (clip W);
- world geometric normal;
- replay-local primary identity;
- a 2.5D motion guide: previous UV − current UV, predicted previous minus current linear depth, and admissibility;
- current and previous camera, reset and sequence.

The backend owns its history. Its output contract is unchanged: RGB and current support confidence, with W<0 invalid. Composition still gates on current raw validity.

The project backend runs the unchanged ADR-011 spatial filter first, as the literal history-free path, then the temporal stage. Game semantics, source ownership, GI generation and composition stay outside the backend.

External backends. FSR Ray Regeneration's indirect-diffuse contract maps onto these inputs through adapter-side conversion only:
- RGBA16F radiance plus hit distance, where negative means inactive;
- linear depth;
- 2.5D motion;
- world normal.

Its required diffuse albedo does not exist in RT+:
- The signal is already demodulated, so at most a labelled constant proxy is admissible. `surfaceResponse`, SHADE and authored colour are never substituted.
- Roughness and material type get documented defaults.
- Per-pixel motion admissibility cannot be expressed, so that backend would own its own disocclusion.

It is DX12/RDNA4-only, while Vulkan is the qualified path. Its evaluation is a separate backend work package against this baseline.

Source collection. This consumer selects GI source candidates per pixel and occurrence, and needs no source identity. The tested binding churn changed no selected candidate. Independent source availability (old WP2) is therefore not a prerequisite. Resume it when either of these appears:
- a consumer that needs source-associated lifetime or identity: source-associated reuse, source presentation, or broad unbound local influence;
- temporal GI evidence of visible artifacts driven by binding-derived population changes.

Rejected:
- Treating Tagged/Content identity with `Continuous` and trusted motion as history validity. The fixture disproves it.
- Extending WP3 toward persistent object identity to make the fixture pass. That is unbounded and unneeded for a location-based signal.
- Integrating a vendor SDK, or moving off Vulkan, before the canonical inputs are proven in-game.
- Temporal-before-spatial ordering for the first backend. It would remove the unchanged spatial path as the exact fallback. It can be reconsidered by a later backend.

Cost. Resources are allocated only while the temporal consumer is active: +32 B/pixel.
- RGBA16F motion guide;
- two RGBA16F history targets, one written per occurrence and read by composition;
- two packed R32 previous guides (FP16 linear depth, 6:6 octahedral normal, 4-bit history length).

## ADR-018 — D3D12 is an RT+ backend qualified on the final image; FSR Ray Regeneration is an optional history backend; receiver combiners accept SHADE-free first cycles

Status: accepted, 2026-09-26; amended the same day (final-image correction, fix 7 below). Extends ADR-004, ADR-011 and ADR-017. Evidence: [INDIRECT_RECONSTRUCTION.md](INDIRECT_RECONSTRUCTION.md), [RUNTIME_VALIDATION.md](RUNTIME_VALIDATION.md).

**D3D12.** The RDNA4 gate that forced Vulkan on drivers up to Aug 2026 (the installed 32.0.31041.1004 is exactly that version) had no reproducing driver defect. The Sep 2026 D3D12 crash was the raster-varying ABI issue fixed under ADR-004. Genuine D3D12 RT+ needed six generic fixes. None is hardware-specific:
1. **Runtime-specialized raster shaders.** DXC's `lib_6_3` validator rejects a scalar entry parameter that reaches the external library call. The D3D12-only runtime-compiled entries now spell scalar varyings `float1` and write `SV_Coverage` through a local. Without this, every specialized shader aborted the process. Vulkan uses specialization constants and never compiled this text.
2. **Post-blend dither PS signature.** It skipped the `TEXCOORD1/2` varyings that RT+ added to `RasterVS`. D3D12 links stages by packed signature order (Vulkan links by location), so those PSOs failed and were bound as null. The ADR-004 consequence ("any new varying requires D3D12 validation") had not been exercised for those varyings.
3. **Plume DXR root signatures.** The state object associated the application layout as a *local* root signature, while recording binds it globally, as Vulkan does. The layout is now the global root signature, exports use an empty local root signature, and shader records hold only identifiers.
4. **RT+ read-write textures.** They were created `STORAGE` only. Plume derives D3D12 UAV capability from `UNORDERED_ACCESS`, so every RT+ UAV descriptor was invalid and removed the device with `DXGI_ERROR_INVALID_CALL`.
5. **Plume `copyTextureRegion`.** It set sample positions on a null texture for placed-footprint (readback) destinations.
6. **Diagnostics.** Failures of PSO creation and command-list `Close` are logged. DRED is available with `PLUME_D3D12_DRED=1`.
7. **Plume D3D12 descriptor heap layout (amendment).** `D3D12DescriptorSet` gave immutable samplers a view-heap slot, while `D3D12PipelineLayout` turns them into static samplers without advancing the table offset. Every view declared after an immutable sampler was therefore written N slots after where the root signature reads it. RT64's common raster set has 18 immutable samplers at bindings 7–24. Upstream raster shaders never read that set above binding 6, so the defect was latent. Enhanced per-pixel lighting reads `rasterLights` (t37) and the normal and world-transform buffers (t68–t70), and on D3D12 got zero lights and zero normals. Only the GI-reconstructed ambient survived, so the final raster was about half as bright, and surfaces went black without GI. Fix: immutable samplers take no heap slot, and `setSampler` ignores them. This is a generic Plume backend bug, still present in upstream Plume `main` (`d723793`), and a separate upstream candidate.

`Auto` keeps Vulkan on RDNA4, because it remains the primary qualified path. An explicit D3D12 choice is honored. D3D12 and Vulkan produce equivalent RT+ signals, within the harness's run-to-run floor, and equal product cost.

**Correction (amendment).** The original qualification compared only intermediate RT+ signals (raw and reconstructed indirect). Those are produced by passes whose descriptor sets have no immutable sampler before a view, so they were correct, while the final raster was not. It did not establish final-image equivalence: a read-only audit (`_working-directory/diagnostics/2026-09-26-visual-regression-readonly/REPORT.md`) measured D3D12 final raster at about 50% of Vulkan's mean luminance in the same state. After fix 7, the same candidate matches Vulkan's final composed raster at 2134×1200 in Clock Town at noon with Atmospheric fog (mean luminance 0.2531 vs 0.2532), at noon with Original fog (0.2466 vs 0.2466) and at 23:00 (0.1981 vs 0.1977). Residual differences are limited to animated actors, flame and flicker-light phase. Backend qualification is now judged on the final composed image as well as on intermediate signals (RUNTIME_VALIDATION.md).

**Ray Regeneration.** AMD FSR Ray Regeneration 1.2 is an optional temporal backend of the bounded diffuse GI consumer, behind `IndirectReconstruction`.
- **Selection:** `RT64_RT_GI_RECONSTRUCTION=rayregen`. It is compiled only with `RT64_FFX_SDK_DIR`, the MIT FidelityFX API headers, which are not vendored. It is loaded at runtime from `amd_fidelityfx_loader_dx12.dll`.
- **Fallback:** if the backend is unavailable (Vulkan, missing DLL, no provider, SM < 6.6), the project temporal backend runs and the reason is logged.
- **Boundary:** the ADR-011/017 boundary is unchanged:
  - The project spatial stage still runs first. Its support confidence is output W, so composition validity and authority are identical across backends.
  - An adapter pass converts only the canonical guides:
    - linear depth = clip W;
    - normals in the vendor octahedral encoding, roughness 1;
    - material channel = canonical signal validity: 0 active, 1 for pixels whose raw W < 0. Without it the vendor mixed their zero RGB into receivers (0.68 energy at the border). It encodes no material semantics.
    - motion: the canonical guide for admissible surfaces, and best-effort camera-only reprojection of the primary hit where the guide is inadmissible. The vendor owns disocclusion.
    - diffuse albedo: the labelled neutral-white proxy. Specular albedo is left zero-initialized, as the contract requires without a specular signal.
  - **View space.** The vendor's view space is +Z-forward. N64 views look down −Z. The adapter mirrors the view in Z and folds the mirror and the trace screen transform into the projection. `view × projection` equals the traced transform, and canonical motion Z needs no conversion.
  - **Projection split.** The projection is `inverse(view) × viewProjection`. It must be a perspective; otherwise that occurrence uses the spatial result and the next vendor dispatch resets. This fails closed for views supplied by mods.
  - **Tuning.** Stability bias defaults to 0.25, overridable with `RT64_RT_RAYREGEN_CONFIG`. The vendor default of 1.0 biased converged GI to 0.86 of the spatial estimate on RT+'s sparse four-ray signal. Pre-exposure and outlier clipping had no effect.

**Evaluation (2134×1200, D3D12, Clock Town noon).**

| | Project temporal | Ray Regeneration |
|---|---|---|
| Composition-input noise, still | 0.00088 | 0.00045 |
| Composition-input noise, fast turn | 0.00122 | 0.00036 |
| Reprojected instability, turn | 0.00090 | 0.00077 |
| Energy vs spatial | 0.997 | 0.98–0.99 |
| Reconstruction cost | 0.40 ms | 3.39 ms (+3.0 ms) |
| Resident memory | about 82 MB | 321 MiB vendor (236 MiB aliasable) plus about 100 MB adapter |

Ray Regeneration also softens real low-frequency contact detail during motion.

It works cleanly behind the boundary and is the better denoiser. At RT+'s current signal budget it is not a better product default: about 8× the cost of the project backend, for a component that is a bounded share of ambient. It remains an optional, qualified experiment.

**Faceting** originates in the signal and the guide, not in reconstruction. GI is gathered over the per-triangle geometric normal, and the normal guide is that same normal (about 11–12° across soft edges on low-poly actors). The absolute edge step is about 0.002–0.006 luminance in every backend, raw included. Lower-noise backends only make it relatively more visible (edge/interior ratio 1.4 raw, 1.9 spatial, 4.1 project, 14.8 Ray Regeneration). Normal semantics were not changed.

**Other signals.** The other Ray Regeneration signals were assessed and not integrated:
- AO would need stochastic per-occurrence sampling of spatial visibility, which today is a fixed deterministic 12-direction term.
- Dominant-light visibility needs cone-sampled shadow rays with hit distance and an angular radius. Today it is a single hard ray.
- Direct diffuse needs a demodulated local direct radiance signal; composition computes local direct analytically.
- No specular signal exists.

Each belongs to the feature that owns the signal, not necessarily to `IndirectReconstruction`.

**Receiver combiners.** The CPU receiver gate `spatialCombiner` was the only active receiver rejector:
- 34 of 460 surfaces in Clock Town;
- **48% of visible RT-hit pixels**, including the large building walls and Link's tunic, which rendered almost black under the sun shadow while neighbouring receivers got GI fill.

The rejected forms were `(TEXEL1·TEXEL0)` or `(TEXEL0·PRIMITIVE)` in cycle 0, followed by `COMBINED·SHADE`. Both are pure products with SHADE. The second is the same product as an accepted form, reordered. A SHADE-free cycle 0 is now accepted, but only as the operand of a following `COMBINED × SHADE`. Its inputs are limited to textures, PRIM/ENV colours and their alphas, LOD fractions, ONE and ZERO. This keeps the final colour a pure product with SHADE: no additive or emissive term, no noise, key or YUV inputs, and alpha still gated by the opaque test. Combiners without SHADE stay rejected. Visible RT-hit coverage rose from 52% to 100% in the capture.

*Amendment:* the "almost black in sun shadow" evidence above was measured on D3D12 before fix 7, when those surfaces had no per-pixel lights at all. On a correct backend, the widening's visible effect is smaller.
- On Vulkan, frozen `cand-f` → `cand-g` in Clock Town at noon: mean luminance 0.2600 → 0.2535, 13% of pixels change by more than 0.05.
- The change is confined to the newly admitted walls, stall fabric and Link. Those surfaces now take RT+ ambient composition instead of authored fill. The sun-shadowed brickwork becomes slightly lower in contrast, and the orange wall a little greyer.
- Exposure, sky, fog and unaffected geometry are unchanged.

This is the expected consequence of the eligibility change, not a regression. Whether that artistic balance is right is part of the still-open cross-scene receiver qualification.

The durable receiver contract is in [SPATIAL_LIGHTING.md](SPATIAL_LIGHTING.md#receiver-and-ownership-boundary): the principle, the per-draw gate, the accepted and rejected forms, and game-agnostic rule vs. MM-only qualification. It is pinned by `rt64_spatial_receiver_fixture`. The widening is independently revertible: the predicate hunk and the fixture's two widened cases.

This is receiver eligibility only. Source authority, historical-light ownership and the all-or-nothing trait bundle are unchanged. The GPU `authoredFillResponse` predicate already accepted these forms.

**Rejected:**
- keeping the RDNA4 gate without a reproducing driver defect;
- an Agility SDK dependency to get debug layers (the diagnosis used DRED and an in-process trace);
- substituting SHADE, `surfaceResponse` or authored colour for the albedo proxy;
- encoding motion inadmissibility as zero motion;
- making Ray Regeneration the default at the current cost;
- per-scene or per-texture receiver exceptions.
