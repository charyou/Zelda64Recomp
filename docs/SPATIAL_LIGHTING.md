# Spatial response, bounded GI and lighting authority

2026-09-24: GI primary bounce illumination and its visibility ray are weighted by the adapter-published primary visibility authority (ADR-014). A non-geometric primary, such as the below-horizon night fill, contributes no bounce transport.

Implemented 2026-09-14. Build/runtime continuation evidence: HANDOFF.md. Source facts: ASTRA_LIGHTING_ADDENDUM.md. RT64 contains generic source/receiver policy only.

## Receiver and ownership boundary
A **receiver** is a surface that RT+ may shade with spatial/indirect response. Eligibility is a statement about how the surface's final colour responds to SHADE. It is not a statement about which lights the surface owns: receiver eligibility is independent of caster membership, source authority and original-light ownership (ADR-011, ADR-013, ADR-018). Accepting a receiver never grants ownership of an original light term.

**Principle.** A surface is a receiver only when its final colour is a *monotonic pure product with SHADE*. Replacing or rescaling SHADE then scales the authored result without changing its artwork, and no additive, emissive or special-arithmetic term can be amplified. Anything that cannot be shown to have that form falls back to the existing rendering.

**Per-draw gate** (CPU, `rt64_framebuffer_renderer.cpp`; first rejection reason recorded in receiver rows):
- The RT scene boundary: opaque, depth-writing, perspective geometry whose projection is the RT world projection. Otherwise `projection_mismatch`.
- World camera. Otherwise `world_camera_mismatch`.
- No RSP-modified vertex RGBA. Otherwise `modified_vertex_color`.
- Unlit authored vertex colour, or a supported authored light set. Otherwise `lighting_fallback`.
- `spatialCombiner` (`shared/rt64_spatial_receiver.h`) accepts the colour combiner. Otherwise `unsupported_spatial_combiner`.

Per pixel, the raster then requires the RT primary hit to be the same instance at the same clip W. A mismatch uses the draw's normal fallback (diagnostic view 13).

**Accepted combiner forms** (colour combiner `(a − b) × c + d`; in 1-cycle mode only the second cycle is evaluated):
- In either cycle:
  - `TEXEL0/1 × SHADE` or `SHADE × TEXEL0/1`;
  - pure `SHADE` (`d = SHADE` with a zero product);
  - `SHADE × PRIM/ENV`.
- Second cycle after one of those products:
  - `COMBINED` pass-through;
  - `COMBINED × PRIM/ENV` modulation.
- First cycle that is entirely **SHADE-free**, followed by `COMBINED × SHADE`. SHADE-free inputs are texels, PRIM/ENV colours and their alphas, LOD fractions, ONE and ZERO. This covers texture LOD/detail blends, texture products such as `TEXEL1 × TEXEL0`, and constant tints such as `TEXEL0 × PRIM`.
  - Widened 2026-09-26 (ADR-018). Before that, only LOD/detail blends were accepted here, which left Clock Town walls and Link's tunic unlit by RT+ indirect.

**Deliberately rejected:**
- no SHADE response at all;
- any additive or emissive term beside the SHADE product (for example `TEXEL0 × SHADE + PRIM`, or `COMBINED × SHADE + ENV`);
- SHADE applied twice (`TEXEL0 × SHADE`, then `COMBINED × SHADE`);
- noise, chroma key (`KEY_CENTER`/`KEY_SCALE`), K4/K5, `COMBINED_ALPHA` or `SHADE_ALPHA` inputs in a SHADE-free stage;
- cutouts, RGBA-modified vertices, ambiguous light sets and non-world cameras (per-draw gate above).

Alpha is not part of this predicate; the opaque boundary gates it. The GPU `authoredFillResponse` predicate (`SpatialLighting.hlsli`) is a separate, already broader check for authored fill. `rt64_spatial_receiver_fixture` (`tests/spatial_receiver_fixture.cpp`) pins representative accepted and rejected forms. Against the pre-widening predicate it fails exactly the two widened cases.

**Scope of the rule vs. its qualification.** The rule is game-agnostic: it reads only RDP combiner and RSP state, with no scene, actor, texture or asset identity. It applies to vanilla and modded content alike. Runtime qualification so far is Majora's Mask only, mainly Clock Town (noon and night, Vulkan and D3D12). It does **not** establish qualification for other games or for heavily modded content.

Future evidence may justify further generic widening, for example other provably monotonic SHADE forms. That would be a separate decision with its own fixture cases; it is not open work.

**Exploratory observations (2026-09-26, human, not deterministic evidence, not diagnosed):**
- Overall the widening looked visually beneficial across several further Clock Town views, including night, with no broad regression seen.
- In several cutscenes, some content appeared not to receive the widened treatment, or only partially.
- Some indoor/shop surfaces, including sheet-like geometry, appeared to receive no GI.
- Outdoors at night, very slight flicker was seen on some geometry, which appeared to alternate between recognized and not recognized.
- During the long intro, especially on very large ground surfaces, it was unclear whether GI/receiver participation was active.

These are future qualification targets, not attributed to the widening.

Each should first be classified with the existing instrumentation: receiver rows and first-rejection reasons, diagnostic views 6/13/14, and RT scene series. The candidate classes are:
- intended eligibility rejection (combiner, modified colour, lighting fallback);
- unstable receiver classification (per-draw traits or per-pixel instance/clip-W match changing between frames);
- geometry missing from the submitted RT scene;
- missing GI support or reconstruction (invalid raw W, low confidence);
- expected fallback for cutscene or non-world cameras.

CPU-only modifiedColor provenance follows RSP loads/copies/edits; lightCount is unchanged. Surface.w capabilities: receiver1, authored-lit2, authored-color4, local8, indirect16. Geometric normals support authored-color response without treating RGB as normals or claiming recovered material properties.
SemanticLight.response.w explicitly authorizes unowned spatial strength (zero denies). MM verified receipts grant .35. Exact frame source snapshots deduplicate, capped64. A receiver ranks four local-direct contributors by finite influence and excludes its bound source terms. Source shadow policy remains independent. Run-4 owned replacement bits/equation are unchanged. Unowned direct is capped .3 before authority gain; combined artistic addition capped .3..45.

## Generation and source authority

ADR-013 applies the permission/ownership distinction to Primary environment direct: source permission and validated spatial eligibility authorize bounded additional influence independently of historical decomposition. Primary color W grants permission (default zero), and existing call/depth/normal guides plus mode bit64 support it without another ray pass. One matching historical term still uses exact replacement; ambiguity preserves fallback. See PRIMARY_ENVIRONMENT_DIRECT.md for its distinct gain-minus-one/headroom composition. Local source budgets/equations, Secondary direct and ambient/fill transfer are unchanged. F9's session master overrides their effective fallback gates together without modifying their configured values.
Four cosine hemisphere samples trace at most one finite diffuse bounce (shared environmentRadius). Pixel-stratified fixed rotation replaces the coarse world-cell pattern. A miss contributes zero transport; nearby openness is not proof of sky. Missing valid environment metadata leaves raw invalid and preserves original indirect responsibility.
Bounce illumination uses resolved authored ambient, explicitly published primary environment directional with RT visibility, and the separate secondary directional as half-strength broad authored illumination. Primary direct visibility requires both direction and RGB agreement with the original bound light; secondary direct keeps its original per-pixel evaluation. Visual sun position is not the lighting source.
Verified local snapshots also illuminate bounce hits: select two by finite-support relevance once per primary receiver, then reuse for four bounces. At most two local evaluations/finite occlusion rays per hit. The generic environment profile grants localBounceStrength (.75 from MM). This is transport from a secondary surface, not reapplication of primary owned direct. Publication currently covers verified bound snapshots, not every scene LightContext source.
A draw-average authored-color or authored-light-sum proxy supplies tint: 25% chroma, maximum .7 response. It is neither textured physical albedo nor emission. Incident clamps1, four-sample mean clamps.5, finite distance fades. No bright-texture emitters, recursive paths or vendor SDKs.

## Replaceable reconstruction
Canonical contract, backend and evidence: [INDIRECT_RECONSTRUCTION.md](INDIRECT_RECONSTRUCTION.md) (ADR-011, ADR-017). Raw RGBA16F = receiver-independent incident indirect RGB + mean bounce hit distance (invalid W=-1). Renderer-owned guides: replay-local call/primitive identity and clip depth, octahedral geometric normal and, while GI history is active, a 2.5D motion guide. The project backend runs the unchanged 7x7 spatial filter (the history-free path) and, by default within GI, a temporal stage whose history is reused only where stored previous depth/normal agree with the reprojected surface; surface identity never authorizes history. Output RGB + current support confidence; raw remains unchanged and independently viewable. A future Ray Regeneration/DLSS adapter replaces the backend, not game/GI/composition semantics.

## Composition
Current / RT+ authority0..1 transfers a confidence-weighted share of supported ambient/fill responsibility; no framebuffer crossfade or global ambient multiplier. Exact authored-lit ambient is separately replaced. Authored-color surfaces declare a 75% artistic fill partition and preserve25% artwork. New fill combines finite enclosure/contact with reconstructed transport, retains an18% floor, caps transported energy relative to authored ambient, and leaves directional direct separate. The old small additive GI applies only to the untransferred share. The old environment lobe is reduced while GI is active; RT+ uses the common enclosure/transport result instead of stacking full approximations.
Unowned local direct gains up to2 with RT+; owned semantic direct retains Run-4 exact replacement. Invalid receivers/guides preserve their known-good responsibilities. Supported pure-SHADE surfaces share the boundary to reduce textured/untextured patchwork; no object/actor/texture grouping hacks.

## Controls and evidence
F1 Lighting: Enhanced spatial local response, Bounded diffuse GI, GI raw/bypass reconstruction, Current / RT+ authority. GI/spatial local default off, authority default1. JSON rt_spatial_local, rt_gi, rt_lighting_authority; F1 edits session-local. Launch RT64_RT_SPATIAL_LOCAL, RT64_RT_GI, RT64_RT_GI_RAW, RT64_LIGHTING_AUTHORITY. Views6 receivers,7 spatial locals,8 raw GI,9 reconstructed GI (radiance diagnostic gain4, not production brightness).
Boundary and GI builds passed; Inn room participation and the user's raw-signal evidence are established. RT+ build2 passed and runs in Inn; matched authority0/1 captures show stronger room shading while artwork remains. Build3 adds safe untextured coverage and passes; Town captures separately show unowned floor and owned Link/stall response plus coherent shaded coexistence. Final build4 passes and runs in daylight. Exact artifacts and limits are recorded in HANDOFF.md.
