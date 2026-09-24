# RT+ Architecture Reassessment

**Date:** 2026-09-24
**Reviewed state:** parent `8893d4a` → `lib/rt64` `ccb86d2` → Plume `91e6711`. Parent, RT64 and Plume are committed as one coherent checkpoint. Only the supplied documents are untracked.
**Upstream reference:** the fork is based on upstream RT64 `5473732`. Upstream `main` (`4337374`) is only two commits ahead: an RDNA4 Vulkan workaround and a VI validity fix. The fork adds about 5.6k lines across 40 RT64 files.
**Posture:** architecture decision basis. No code was changed. See §8 for why no candidate qualified as an opportunistic fix.

Status: historical review snapshot. `DECISIONS.md` remains authoritative for decisions and `HANDOFF.md` for the current state.

Evidence basis: current source in the parent, RT64, N64ModernRuntime and Plume; the fork diff against `5473732`; the current contracts, ADR-001…013, HANDOFF, CHANGELOG-INTERNAL, the 2026-09-15/17 reviews, `AGENTS_RT64.md`, the Vision plus its addendum, `ASTRA_LIGHTING_ADDENDUM.md` and the MM decomp (`G:\_Development\Github\mm`) where a specific fact was needed. No separate "Project Context / Intent" or forward-technology research file exists in the repository, so the task brief itself served as the intent statement. `docs/input-research/` is older (2026-09-09) renderer research. Line numbers refer to the reviewed commits.

---

## 1. Executive assessment

The **lighting-responsibility model** is coherent and should stop being reopened. It rests on these separations:

- source vs receiver vs visibility vs transport vs reconstruction vs composition;
- owned replacement vs permitted increment;
- per-responsibility fallback;
- Native as the reference.

ADR-010 to ADR-013 converge on one principle that is genuinely reusable: historical ownership authorizes *removal*, while source permission together with receiver semantics authorizes *modern influence*. No MM identity has leaked into the shading policy.

The main risk is **not** in that model. It lies in three foundations underneath it. Each was built as the minimum needed by the first consumer, and each has since become a de facto permanent contract:

1. **The RT scene is a by-product of raster replay, not a renderer concept.**
   - It is rebuilt from scratch for every framebuffer × Workload replay × HFR occurrence.
   - It is populated only with what the game submitted and culled this frame, anchored to the *first* qualifying projection.
   - Its surface identity is replay-local.
   - Stock RT64 already provides cross-frame identity (frame matching, `gEXMatrixGroup` transform groups, previous transforms, `worldVelBuffer`). RT+ uses none of it.
   - The primary ray window is also mis-derived for N64 GL-style clip space (§3, F1), so receivers are covered only within a bounded distance band.
2. **Source availability is defined by raster binding occurrence.** A semantic source exists for RT+ only if some draw in this Workload bound it into one of MM's seven RSP slots. That binding already passed MM's per-actor nearest-seven selection and frustum/radius culling. Sources are then deduplicated by value, capped first-come at 64, and have no identity. The `SemanticLight` record itself is correctly slot-independent; the *collection* is not.
3. **Semantic responsibility metadata exists for some source classes but not others.**
   - Local sources carry shadow authority, range and permission.
   - Environment directionals carry only direction and RGB, plus a permission scalar stored in `color.w`.
   - Nobody owns "should this directional be geometrically shadowed at all". ADR-012 correctly kept elevation policy out of RT64 but gave it to no one. At night the published primary (`dirLight1`) is a weak light pointing *below* the horizon, yet RT+ traces geometric visibility toward it. Meanwhile the secondary (`dirLight2`, the upward, moon-side light in time mode) is never shadowed.

The visible failures in the brief map onto these foundations rather than onto the lighting equations:

| Failure | Foundation(s) it most plausibly depends on |
|---|---|
| Camera-motion shadow instability | Scene population and projection selection (1), primary depth window (F1); the cause is not yet separable (§6) |
| GI/environment flicker | Replay-local identity, no motion/history, screen-fixed sample pattern without a frame seed, value-deduplicated and binding-dependent local contributors that appear and disappear (1, 2) |
| Night directional shadows | Missing environment-source visibility authority (3) |

**Is consolidation warranted before major new features?** Yes, but it should be targeted, not a refactor. Before temporal reconstruction, richer GI, emissive surfaces or source presentation are added, the project needs four things:

- a small scene/identity foundation;
- a source collection separate from ownership receipts;
- an explicit per-source responsibility record for environment directionals;
- the correction of two concrete coverage defects.

Each future feature needs at least one of these, and adding features first would multiply the places that encode today's replay-local assumptions.

---

## 2. Strong foundations — stop reconsidering

**Stock RT64 foundations RT+ correctly reuses**
- **Deferred Workload replay plus GPU RSP processing.** HFR frames are real renders with interpolated transforms. RT+ traces the same `worldPosBuffer`/`faceIndicesBuffer` that raster uses, so raster and RT agree within an occurrence by construction (`rt64_framebuffer_renderer.cpp:2057`).
- **Extended GBI as the draw-ordered channel.** `gEXSetLightSource` fits the stock design principle exactly (append-only id, fallback when absent).
- **Plume as the only backend layer.** The three fork Plume commits (AS build contracts, ranged queries, Vulkan cropped readback) are generic. One caveat: `28f2fe9` adds a pure-virtual method to `RenderQueryPool`, which any out-of-tree backend would need to implement.
- **Leaving legacy `RT_ENABLED` compiled out.** Do not resurrect the historical DI/GI/denoiser stack.

**RT+ contracts that should become durable**
- **Ownership vs permission (ADR-013)** as the cross-cutting principle.
  - Slot matching is the *correct* use of the 7-slot realization: it establishes which authored contribution may be replaced.
  - It should never again be the gate for source *availability*.
- **Per-responsibility fallback, with Native as the reference.** Responsibilities are independently disableable, and the F9 master override is an *effective* override that never rewrites configuration. Both are the right shape for future games with weaker adapters.
- **Receiver-validation discipline.** Raster accepts RT data only on matching call identity and clip depth. This rejection-not-darkening rule is what makes RT+ safe on overdraw, transparency and unsupported content.
- **One shared RT scene per framebuffer, shared by all consumers**, with sequential queries (recursion depth 1) and one `SurfaceHit` convention. Do not add a second scene owner.
- **No emission or light-source inference from appearance.** Bright ≠ emissive and glow ≠ light are enforced in the code: the only emitters are published `SemanticLight`s.
- **Raster linkage-ABI discipline (ADR-004).** Varying additions have been rare and qualified.
- **Data separation of raw signal / guides / reconstruction result / confidence.** The *data* boundary is right, even though the *code seam* is missing (F8).
- **Instrumentation posture.** It observes production decisions and fails closed. It stays optional and never feeds policy.

**Divergences that are justified and should not be reopened**
- A minimal RT owner instead of legacy RT.
- Per-pixel evaluation of actual RSP light semantics (ADR-007).
- The semantic local source record via GBI (ADR-010).
- Primary environment RGB treated as an energy reference with bounded gain (ADR-012).
- Enhanced-only modern fog with a Native reference (ADR-001/003).

---

## 3. Significant architecture findings

Each finding lists: current state, why it exists, why it matters, evidence, and whether it is real debt or ugliness.

### F1 — Primary rays cover only part of the depth range (concrete defect)

**Current state.**
- `PrimaryRayGen` unprojects NDC z = 0 and z = 0.99 through `inverse(modViewProj)` (`src/shaders/PrimaryHitRT.hlsl:261-268`).
- N64 `guPerspective` is GL-style: clip z ∈ [−w, w], with `mf[2][2] = (n+f)/(n−f)` and `mf[3][2] = 2nf/(n−f)`.
- `RSPProcessCS` keeps that convention (`ndcPos = tfPos.xyz / w`).
- NDC 0 is therefore view depth ≈ 2nf/(n+f) ≈ **20 units**, not the near plane. NDC 0.99 is ≈ 2nf/(0.01f + 1.99n): **≈1,730 units** at MM's zFar 12,800, and ≈1,335 at zFar 4,000.
- MM uses zNear = 10 and zFar = `lightCtx.zFar` (clamped to 12,800) (`mm/src/code/z_play.c:1178-1183`, `z_view.c:39`).

**Why it exists.** Run 1 inherited a D3D-style "0 = near" assumption for a diagnostic inset. The contract even says "no promise of full far-plane coverage", but the actual window is far narrower than that sentence suggests.

**Why it matters.**
- Every receiver-gated responsibility is silently absent outside a camera-relative distance band: sun visibility, AO/enclosure, locals, unowned direct, GI and primary expansion. Casters beyond the band still occlude, but *receivers* there get no guide and fall back.
- This produces distance-dependent enhancement boundaries that move with the camera, which is consistent in kind with part of the camera-motion shadow reports.
- It is the cheapest confound to remove before diagnosing culling.

**Classification.** Real correctness defect with a trivially local fix. It is not fixed here because it broadens production coverage everywhere and needs runtime qualification (§8).

### F2 — Receiver clip-W reconstruction is probably wrong on D3D12 (defect, needs confirmation)

- The receiver gate uses `clipW = 1.0f / SV_Position.w` (`src/shaders/RasterPS.hlsl:151`).
- That is correct for Vulkan, where FragCoord.w = 1/w.
- On D3D, SV_Position.w in the pixel shader is w itself. `CMakeLists.txt:117` passes no `-fvk-use-dx-position-w`, and the HLSL does not branch per target.

**Consequence.** On D3D12 almost no RT receiver should validate, so every RT+ responsibility falls back. D3D12 runtime has never been qualified, so this would have gone unnoticed. ADR-004's fog-depth reconstruction relies on the same convention and should be checked together.

Confirm with one D3D12 run of view 13 or the guide-acceptance count before changing anything.

### F3 — Source availability is conflated with raster binding (the "seven-light" question)

**Current state.**
- The RT source list is built by walking the `rspLights` snapshots that draws in this Workload loaded (`rt64_framebuffer_renderer.cpp:1540-1597`).
- A source is present only if MM's `Lights_BindAll` bound it for some drawn object. That bind already applied:
  - the per-object nearest-seven selection;
  - view-frustum and radius culling (`mm/src/code/z_lights.c:86-91, 144-146`);
  - the exact-realization receipt check (`patches/semantic_lights.c:53-80`).
- Skyless rooms bind with `refPos = NULL` and are rejected by `owns_point` (`semantic_lights.c:65`).
- The collection is then deduplicated by `memcmp` of value, capped first-come at 64 (`:1560`), and ordered by traversal.
- No source identity exists: `bindOrdinal` is frame-local and lineage is capture-only.

**What is right.**
- `SemanticLight` is slot-independent.
- Owned replacement is slot-scoped, and that is correct.
- The unrolled 7-iteration loops, the 2-bit×7 visibility packing and the ownership bits 21–27 correctly bound *ownership of original contributions*.
- None of that needs to change.

**What is historical realization masquerading as semantics.**
- Whether a source *exists* for modern Direct/GI depends on whether some nearby object happened to draw and bind it this frame.
- A torch behind the camera, or one near no bound receiver, cannot contribute GI or shadow anything.
- Its presence flips as the camera moves, actors cull and bind order changes.
- The 4-candidate unowned and 2-candidate GI selectors are sensible *budgets*, but they select from the wrong population.
- Value dedup also merges distinct sources that happen to have identical values. Conversely, a flickering source is a new "source" every frame.

**Why it matters.**
- This blocks broader local lighting and stable GI from local sources.
- It blocks temporal reuse of source sampling and source presentation (glow↔source association).
- It is a plausible contributor to GI flicker.
- ADR-010 already anticipated "a collection independent of RSP replacement slots"; nothing implements it.

**Classification.** Real architectural debt. The original game genuinely distinguishes authored sources (`LightContext` `LightNode`s with position, RGB and radius) from their per-object binding, and RT+ currently only sees the latter.

### F4 — Environment directionals lack source-level visibility and responsibility authority (night shadows)

**Current state.**
- The primary is `envCtx.dirLight1` (`patches/play_patches.c:224`). Validity is any finite direction plus max RGB > 0 (`rt64_framebuffer_renderer.cpp:1467-1471`).
- Shadow rays run whenever the primary is valid and shadows are on (`PrimaryHitRT.hlsl:294`), regardless of energy or elevation.
- In MM time mode, `light1Dir.y = cos(t − 12h)·120` (`mm/src/code/z_kankyo.c:1394`), so the primary points **below the horizon at night**. ADR-012's own 23:00 capture recorded "(40,50,60)/255 and valid negative-Y direction".
- `dirLight2 = −light1Dir` (`:1398`) is the upward light at night. It is **never** shadowed, never expanded, and gets only half-strength GI.
- The GI bounce shadows the primary independently of `modes.z` (`PrimaryHitRT.hlsl:222`). This is documented intent, but it means the same semantic question is answered differently by two consumers.

**Separating the concepts the brief lists, for a night frame:**
- **Source validity:** yes.
- **Source energy:** low but nonzero.
- **Authored contribution:** a dim fill from below that the original never shadowed.
- **Modern Direct authority:** full (gain ≈1.18).
- **Ray executed:** yes, into the ground.
- **Image contribution:** that authored fill is removed from any surface whose normal faces the below-horizon direction and whose shadow ray is blocked, which in practice is most such surfaces. The relative contrast is visible because ambient is also low.

A valid direction is therefore acting as permission for geometric visibility, which is exactly what the brief says must not happen.

**The authored constraint being discarded.** MM's own directional-shadow logic requires an upward direction and weights by `(R+G+B)·|dir.y|` (`z_actor.c::ActorShadow_DrawFeet`, per `ASTRA_LIGHTING_ADDENDUM.md`). That is game-level evidence of which directional has shadow authority. ADR-012 correctly refused an elevation branch in generic RT64, but then no layer owns the question.

**Better responsibility boundary.** Give environment directionals the same responsibility record locals already have: role, visibility (shadow) authority, unowned permission and energy. The MM adapter derives visibility authority from MM's own semantics (ActorShadow-style elevation × RGB weight, and light mode). RT64 consumes a generic scalar. The secondary (or any N-th directional) then becomes eligible for the same treatment by data, not by special case.

This also removes the `primaryColor.w`-as-permission overload. The adapter's `.35` is currently hardcoded in C++ for every light mode (`src/main/rt64_render_context.cpp:492`), including fixed-light interiors where `dirLight1` is not sun-like.

**Classification.** Real semantic-contract gap. Small to fix once the record exists.

### F5 — The RT scene has no lifetime or identity of its own

**Current state** (`rt64_raytracing_debug.cpp:93-227`, `rt64_framebuffer_renderer.cpp:1982-2102`).
- Rebuild frequency:
  - A full BLAS/TLAS rebuild happens on every `recordFramebuffer` (single identity instance, no refit) for each framebuffer, Workload replay, HFR display frame and held/paused replay.
  - At 120 Hz over a 20 Hz game, the same topology is rebuilt about six times per game frame.
- Per-record allocation: surface, source, response and environment buffers are reallocated every time.
- Projection selection:
  - The first qualifying draw fixes the projection; later draws on another `transformsIndex` are dropped as `projection_mismatch`, even if they carry identical matrices.
  - A non-world camera or an agreement failure silently disables every consumer for that frame.
- Surface record: (call, indexStart, faceCount, receiverTraits), replay-local.
- The fork repurposes stock `raytracingEnabled` to mean "any RT+ request". Frame matching and the VertexProcessor now run every frame by default (`rt64_workload_queue.cpp:225, 519, 1240`). The primary-permission default also keeps tracing active on RT hardware even with all user-facing RT toggles off.

**Why it exists.** It was the minimal Run-1 owner. Sharing it across consumers was the right call, and it held.

**Why it matters.** Several future needs are blocked by the absence of a scene record, not by missing algorithms:
- BLAS reuse (build once per Workload, refit per HFR occurrence is available by construction, since topology is identical);
- per-pixel motion;
- stable identity for history;
- caster persistence policy;
- per-framebuffer resource keying (currently by ordinal position);
- honest HFR cost.

Stock RT64 already has the pieces:
- `GameFrame::match` with transform-group IDs, which Zelda64Recomp tags extensively (`patches/camera_transform_tagging.c`);
- `prevWorldTransforms`;
- `worldVelBuffer`, computed every frame but read only in dead `RT_ENABLED` code (`:355`).

**"Persistent geometry" is not established as the fix.** What is established is that no policy about scene membership *can* be expressed today, because scene membership is an emergent property of a raster loop.

**Classification.** Real architectural debt. It is the most consequential finding for forward work.

### F6 — The shader ABI encodes responsibilities implicitly (accreting fields and bits)

The real problems, as opposed to cosmetic ones:

- **`modes.w != 0` means "produce guides".** Bit 64 is tested by no shader; it exists only to make the word nonzero. Diagnostic view bits (`<<8`) also enable tracing and guides (`:1481`, `PrimaryHitRT.hlsl:281`).
- **One authority field serves several roles.** `lightingAuthority.x` is the GI replacement weight, the unowned-local gain and the unowned cap interpolant (`RasterPS.hlsl:186, 234, 239`).
- **Color alpha carries permission.** `SemanticLight.response.w` is both gate and energy multiplier. `primaryColor.w` carries permission but has no GPU reader; the used copy is `lightingAuthority.z`.
- **Fields have been reassigned or left dead.**
  - `environment[4].y` was reassigned from authority to GI candidate budget.
  - `environment[4].w` duplicates `lightingAuthority.y`.
  - `environment[5]`, `shadowSun.xyz` and `[0].w`/`[1].w` are dead.
- **Other overloaded fields.**
  - `spatialTuning.y` is enclosure radius, GI bounce length and candidate-range margin at once.
  - Receiver and GI-reflector eligibility share the `SpatialIndirect` bit, and traits are always assigned as one bundle.
- **No layout guard.** No `static_assert` covers the 96-byte `RSPLight`, which relies on `-fvk-use-dx-layout`.

**Composition-level duplication.**
- Sky/fill is approximated three times: the hemisphere lobe (halved under GI), the GI ambient replacement, and 0.6·ambient inside GI incident light.
- GI enters twice, as replacement with weight a·confidence and as a capped additive term with weight 1−a, using different scale factors.
- A fixed 25/75 SHADE partition appears in two places (`RasterPS.hlsl:187, 221`).
- Lit-geometry GI tint is ambient + 0.3·Σ light colors (`:2081-2084`), which uses light energy as reflectance and makes the bounce roughly quadratic in energy.

Each of these was a deliberate bounded step. Together they mean no single place states what a responsibility's authority is.

**Classification.**
- The field reuse is worthwhile cleanup. It becomes real debt the moment temporal history or a second reconstruction backend needs to know "which responsibilities are active and what they require".
- The composition duplication is acceptable *art-direction* debt for now. It should be collapsed only when GI replaces the stylized fill and has earned that ownership (ADR-011's own condition).

### F7 — "No visibility" means different things for replacement and for increments

**Replacement case.** For owned replacement, visibility = 1 when unavailable is correct: it reproduces the authored term.

**Increment case.**
- Primary expansion requires receiver guides but **not** visibility. `sunVisibility` is loaded only when `shadowSun.w`, i.e. when shadows are enabled (`RasterPS.hlsl:148-183, 253`).
- The default configuration is `rt_shadows = false`, `rt_primary_direct = true`, authority 1, expansion on (`ultramodern/config.hpp:79-93`, `rt64_workload_queue.h:89`).
- In that configuration, unowned receivers receive the primary increment unshadowed: up to +0.15 × headroom, in occluded interiors too.
- The Inn evidence showing zero increment at 3,148 blocked pixels was captured with visibility enabled. It does not describe the default path.
- Separately, "zero primary matches" is treated as "no authored primary contribution". A near-miss match (modified light color, quantization, mods) therefore receives both the authored term and the increment.

**Why it matters.** This is the "valid direction ≠ meaningful daylight" risk re-entering through a fallback. It also shows a general rule that is currently implicit: fallback for *replacement* may default to authored behaviour, but fallback for an *addition* must default to **no addition**.

**Classification.** Real policy inconsistency needing a decision (§8). It is not a bug by the letter of the ADR-013 text.

### F8 — The reconstruction backend is replaceable in data but not in code

**Current state.**
- `Reconstruction` is a concrete struct inside `RaytracingDebug` with a fixed 4-slot descriptor set (`rt64_raytracing_debug.h:47-65`).
- It is dispatched inline when bit 16 is set; the consumer picks the result or raw by that same bit (`rt64_framebuffer_renderer.cpp:1327`).
- The filter ignores call/primitive identity, so it blends across draws with similar depth and normal.
- There is no frame seed (`PrimaryHitRT.hlsl:200`); the pattern is fixed per pixel, so it crawls under camera motion.
- Missing canonical inputs for any realistic temporal or vendor backend:
  - motion vectors (stock `velFloats`/`worldVelBuffer` exist but are not wired);
  - previous depth, normal and view-projection;
  - history/moment buffers;
  - stable surface identity;
  - sample index/jitter;
  - disocclusion signal.
- A demodulation input is also missing. The project has no clean albedo and should not invent one, so demodulation must use RT+'s own appearance proxy, explicitly labelled.

**Classification.** Real debt for the temporal phase; harmless for today's spatial fallback. Most missing inputs depend on F5.

### F9 — The semantic publication boundary has correctness edges, not just naming debt

The 2026-09-15 review flagged naming. The source shows more.

**Transport path.**
- The data passes through five copies and three naming schemes (fog → atmosphere → `environment[i]`).
- The bridge struct is read by word index with no version (`src/game/recomp_api.cpp:312-371`).
- It is published only from `Play_Main` (`play_patches.c:183-235`).

**Latent frame skew.**
- It is a latest-value mailbox read at `send_dl` (`rt64_render_context.cpp:28-72, 476-540`).
- If a graphics task is dequeued after the next `Play_Update`, it renders with the next frame's environment.
- The game thread normally waits for the task (`patches/input_latency.c:256`), so this is latent rather than observed.

**Stale outside Play.** `valid` is never cleared, so non-Play states inherit the last Play environment for fill and primary. Fog is protected by its own camera/signature checks.

**Lost after a mid-task FullSync (verified).**
- Workload metadata (`fogMode`, `perPixelLighting`, `atmosphere`) is latched once in `processDisplayLists` (`rt64_application.cpp:489-493`).
- `fullSync` advances to a new Workload whose `begin()` → `reset()` clears it (`rt64_state.cpp:2127-2176`, `rt64_workload.cpp:26-28`).
- Any task with more than one FullSync loses all Enhanced metadata after the first. That is harmless for current MM, but it is a portability trap for other games and mods.

**Not interpolated for HFR.** Environment, sources and the atmosphere inverse view-projection are not interpolated, while geometry is.

**Two channels with different timing.** Sources travel draw-ordered through GBI; the environment travels out-of-band through a host mailbox.

**Why it matters.** This is exactly the boundary another recomp must implement. ADR-002 chose host-side Workload metadata for frame-global data. The FullSync loss and skew are concrete reasons to revisit *how* that metadata is carried (with the task or display list), not *what* it contains.

**Classification.** Real architectural debt with two latent correctness defects.

### F10 — Configuration has no owner or precedence model in the renderer

**Current state.**
- All 16 `rt_*` settings live in N64ModernRuntime's generic `GraphicsConfig` (`ultramodern/config.hpp:79-94`).
- One new setting touches about 11–13 files in three repositories, with defaults repeated in three or four places.
- There is no precedence model:
  - launch overrides are applied only in the constructor;
  - any Graphics apply rewrites all 16 atomics (`rt64_render_context.cpp:234-251, 565`), wiping launch and F1 values for those fields;
  - session-only atomics survive;
  - this is documented as intended, but it is "last writer wins", not a layer model.
- Stock RT64's `UserConfiguration`, `EnhancementConfiguration`, `GameConfiguration` and preset libraries are all bypassed.

**MM profile values are scattered.**

| Value | Location |
|---|---|
| `.35` primary permission | C++ adapter |
| `.75` local bounce strength | MIPS patch |
| `.35` source permission and `/160` shadow scale | MIPS patch |
| Atmosphere defaults | four places |

**MM tuning inside generic RT64.** `RasterPS.hlsl:319` (dawn along +X) and `:327` (constants calibrated on the MM intro/forest).

**Classification.**
- Worthwhile consolidation now.
- Real debt for multi-game reuse: a second game would have to fork N64ModernRuntime config to change RT+ defaults.

### F11 — Renderer boundary choices that would make a replaceable renderer hard

These are not reasons to design a plugin API now, only choices to stop deepening:

- The parent includes internal `hle/rt64_application.h` and `hle/rt64_workload_queue.h` and writes about 20 `WorkloadQueue` atomics directly.
- The game API layer calls the RT64 instrumentation singleton.
- `zelda_render.h` exposes RT64 types.
- The RT64 atmosphere override mask mirrors the unversioned mod ABI bit-for-bit (`include/z64recomp_atmosphere_api.h` has no size or version field).
- `AtmosphereParameters` hardcodes exactly two environment directionals.
- Instrumentation hardcodes the parent's `_working-directory` path and "Zelda64Recomp" identity.

The stable cut point already exists: `RendererContext` plus Extended GBI. RT+ has grown a second, undeclared surface beside it.

---

## 4. Historical-constraint findings

**Authored semantics correctly preserved (category 1 — keep)**
- Slot-scoped ownership for replacement.
- Authored hue, relative strength and zero energy.
- SHADE treated as appearance, not albedo (as a principle; F6 notes leaks in GI tint).
- No emitter inference.
- Native/fog/cutout compatibility paths.
- Receiver rejection instead of darkening.
- The artistic fill partition declared as art, not physics.

**Historical realization constraints that became modern contracts (category 3 — should evolve)**
1. **Source availability equals raster binding** (F3). MM's per-object nearest-seven selection and frustum culling were bandwidth and light-count constraints. RT+ inherits them as "what sources exist".
2. **The RT scene equals this frame's game-culled raster submission** (F5). Raster-era culling is a performance realization. RT+ uses it as its caster universe without any policy. This is not proof that persistence is required, but it is a constraint nobody chose.
3. **Two environment directionals with asymmetric modern treatment** (F4). The primary gets visibility, expansion and full GI; the secondary gets only half-strength GI. This reflects the order in which features were built, not a semantic difference. At night it is backwards.
4. **Per-slot-index light-set equivalence.** A reordered but otherwise identical set falls back (`rt64_state.cpp:1235-1247`). This is minor and acceptable.

**Earlier safety boundaries that may now be too restrictive (category 2 — revisit with evidence, not now)**
- Draw-level light-set eligibility.
- Scalar caps (.15 expansion, .3 unowned locals, .5 GI mean, .18/.35 floors).
- The 2/4 candidate budgets.
- Excluding cutouts/alpha from casters.

These are appropriate until temporal reconstruction and a source collection exist. After that, the budgets should be justified by cost and stability, not by the absence of history.

**Modern compute spent reproducing a limitation without benefit**
- Full BLAS/TLAS rebuilds for every HFR occurrence of identical topology, plus per-record buffer reallocation.
- Geometric shadow rays toward a below-horizon fill light. This spends rays to *remove* authored fill with no semantic basis.
- Frame matching and the VertexProcessor forced every frame by the repurposed `raytracingEnabled`, even when no temporal consumer exists.
- The full-resolution primary trace with default settings is not wasteful in itself, but it is effectively always on because the default primary-permission value counts as a consumer.

**The opposite failure: discarding authored intent**
- **Directional shadow authority.** MM's own `ActorShadow` elevation/RGB rule is not represented (F4).
- **Unshadowed primary expansion by default** (F7). This adds modern light the authored scene never had, in places the authored rig lit only through ambient.
- **A scene-constant `.35` permission for all light modes**, including fixed interiors where `dirLight1` is not a celestial source. Whether a directional is sun-like is semantic and belongs to the adapter; the scalar strength is configuration.

---

## 5. Stock ↔ RT+ divergence findings

| Area | Assessment |
|---|---|
| Minimal RT owner vs legacy `RT_ENABLED` | **Justified.** Keep the legacy code compiled out and untouched; deleting it would enlarge merge conflicts in `rt64_framebuffer_renderer.cpp`. |
| Temporal identity (GameFrame match, transform groups, prev transforms, `worldVelBuffer`) | **Unnecessary bypass.** This is the most valuable unused stock mechanism. RT+ should consume it rather than invent surface IDs. Stock computes `worldVelBuffer` and RT+ ignores it. |
| `raytracingEnabled` semantics | **Risky divergence.** The fork redefines a stock flag and thereby changes stock matching/VertexProcessor behaviour. Use a separate RT+ requirement flag so upstream semantics stay intact. |
| Configuration | **Bypass.** Stock `UserConfiguration`/`EnhancementConfiguration`/`GameConfiguration`/presets are unused, and RT+ settings sit in N64ModernRuntime. An RT64-owned RT+ configuration struct, modelled on `EnhancementConfiguration`, is the natural home. Reuse stock's pattern, not its dormant `estimateSunLight`/`lightManager` policy. |
| Upscaler interface (`rt64_upscaler.h`) | **Potentially reusable later.** It is abstract with no implementation. If a joint denoise/upscale backend (Ray-Reconstruction-style) is ever adopted, align with this interface rather than adding a parallel one. Do not let it shape the current reconstruction seam. |
| Extended GBI vs host mailbox for frame semantics | **Worth reconciling.** Stock's rule is that ordering-sensitive information travels in the display list. ADR-002 chose the host side. F9 supplies the evidence to revisit that. |
| Plume | **Clean.** The only watch item is the new pure virtual in `28f2fe9`. |
| Upstream uptake | **Practical now.** The fork is only 2 commits behind. The upstream RDNA4 Vulkan workaround (`8be17e7`) should be evaluated against the fork's own RDNA4 fog fix (`c8ce62b`). The cost of staying current rises mostly with churn in `rt64_framebuffer_renderer.cpp`, `rt64_state.cpp` (+575 lines, largely F1 UI) and `RasterPS.hlsl`. |

---

## 6. Forward-path pressure results

| Future direction | Supported today | Needs redesign first |
|---|---|---|
| **Persistent RT scene / BLAS reuse** | Shared scene owner; world positions from stock HFR; sequential queries | A scene record with identity and lifetime (F5); per-Workload build plus per-occurrence refit; membership policy (only after evidence from §7) |
| **Temporal reconstruction** | Separate raw/guides/result/confidence; call/primitive/clip-W guides | Motion (wire stock velocity/prev transforms per display occurrence); stable surface IDs; history validity; frame seed; reconstruction interface (F8) |
| **Multiple reconstruction backends** | Data separation is right | A code seam plus a canonical input set. Spatial stays as fallback. Vendor adapters project RT+ traits; they do not define them |
| **Richer GI / path transport** | `SurfaceHit` + response buffer + per-hit source evaluation; the ownership/permission model scales to more transport | Source collection with IDs (F3); an honest per-surface response proxy instead of light-sum tint (F6); an emissive-geometry contract; persistent transport state needs identity (F5) |
| **Additional games** | Receivers, visibility, AO/contact, per-pixel lighting and fallback are generic | Renderer-only tier is thin: without environment publication `validPrimary` is false and GI raw is invalid. The semantic packet must be carried with the task (F9), versioned, and allow N directionals |
| **Configuration/profile** | Settings already flow to one place (`WorkloadQueue`) | An RT64-owned RT+ config with precedence (F10); an adapter-owned profile for semantic permissions |

**Reconstruction candidates, classified only where they teach something**
- **ReSTIR-style source/path reuse and temporal denoisers (SVGF/ReBLUR-class):** require missing renderer primitives first. They need F5 identity and motion, and ReSTIR additionally needs F3's source IDs. They are valuable as individual algorithms (reservoir reuse over a bounded source set fits the existing candidate budgets well).
- **Ray-Reconstruction-style joint denoise/upscale:** potentially useful later behind an adapter. It will demand roughness/albedo-like inputs. The rule stands: supply them only as explicitly labelled RT+ appearance proxies, never as recovered PBR truth. It should not influence the current architecture beyond keeping the upscaler seam available.
- **Current spatial filter:** keep it permanently as the no-history fallback. Add call-identity rejection when it is next touched.

**Glow / bloom / emissive — three contracts, none blocked today**
1. **Bloom** is renderer-owned post-processing. It needs no semantics and must never feed lighting.
2. **Semantic source presentation** means the adapter associates a published source ID with the draws that present it (flame, lamp mesh). This requires stable source IDs (F3) and a draw tag (GBI). It can drive glow halos and prevent double counting.
3. **Emissive geometry** means an explicit per-draw GBI authorization carrying radiance and transport participation. It is never inferred from texture brightness. It would join the RT source set as an area source and needs a selection budget like locals.

The current `SemanticLight` record and ownership model accommodate all three. Nothing built so far blocks them. They all depend on F3's source identity.

---

## 7. Missing evidence (only what blocks a conclusion)

1. **Camera-motion shadow loss is not attributable yet.** The candidates are:
   - (a) world-camera agreement failure (the only whole-frame disable), for example during MM's deferred camera update (`z_play.c:1429-1432`);
   - (b) game-culled caster changes;
   - (c) first-draw projection selection or re-issued projections dropping geometry;
   - (d) the F1 depth window.

   **Smallest missing observation:** a per-display-frame *counter series* over about 120 consecutive frames of a reproducing camera pan, with no images. It should record:
   - selected projection index;
   - agreement pass/fail with distance and dot;
   - submitted mesh and triangle count;
   - `projection_mismatch` count;
   - receiver-guide acceptance fraction;
   - primary-miss fraction inside the raster depth range.

   Most of these values already exist as per-draw instrumentation rows (`rt64_framebuffer_renderer.cpp:2125-2129`); what is missing is per-frame aggregation over a *moving* sequence. Stage 4 captures held occurrences only. Fix F1 first so (d) is removed as a confound.
2. **GI flicker split.** No new instrumentation is needed. Use the existing fixed-repeat hold in two runs:
   - held Workload with HFR interpolation only (camera static in game time);
   - live motion.

   Flicker under hold implicates sampling/identity/HFR; flicker only live implicates scene or source population.
3. **D3D12 receiver acceptance (F2):** one D3D12 frame showing view 13 or the guide acceptance count.

Nothing else blocks the work packages below.

---

## 8. Consolidation / simplification opportunities (not implemented)

**Opportunistic correctness fixes: none made.** Four candidates were examined against the rules:

| Candidate | Why it was not fixed |
|---|---|
| F1 depth window | Local fix, but it broadens RT receiver coverage across all consumers and needs runtime visual qualification |
| F2 D3D12 clip-W | Requires a D3D12 runtime this review cannot validate, and it shares a convention with ADR-004 fog |
| Graphics-apply override wipe | Documented behaviour; changing it is a configuration-precedence decision |
| FullSync metadata reset | Latent for MM; the right fix (carry metadata with the task) is part of F9's boundary decision |

The first work package can make all four changes immediately.

**Consolidations worth doing, with their benefit:**
- **Explicit per-framebuffer RT requirement mask** (guides, primary visibility, spatial, locals, GI, diagnostics) replacing "`modes.w != 0`", bit 64 and the use of debug bits as triggers. It makes cost and consumers auditable, lets the no-consumer rule be exact, and keeps stock `raytracingEnabled` meaning intact.
- **Per-source responsibility record for environment directionals** (role, visibility authority, unowned permission; energy stays in RGB). It retires `color.w` as permission and the dead `environment[0|1].w`, `[5]`, `shadowSun.xyz` and `[4].w`.
- **Split `lightingAuthority.x` roles and `spatialTuning.y` roles** into named fields.
- **Split receiver vs reflector traits.**
- **Add a static_assert for `RSPLight`** and fix the stale header comments (`spatialFlags`, `pixelLighting.w`).
- **Rename `RaytracingDebug`** to its production role (carried forward). Do it together with the scene-record work, not separately.
- **Move diagnostic views behind a specialization constant or a separate pipeline variant**, and stop diagnostic selection from requesting RT work.
- **Documentation hygiene.**
  - `HANDOFF.md` still says changes are uncommitted, but they are committed.
  - `RAYTRACING_FOUNDATION.md`'s description of the surface record (projection index) and its NDC-coverage wording are inaccurate.

---

## 9. Recommended next work packages

Ordered by dependency and leverage.

### WP1 — Receiver coverage correctness and scene-population evidence *(direct implementation)*

- **Problem:** F1 and F2 silently remove RT+ from large regions or platforms; the camera-motion cause is not attributable.
- **Why now:** it is cheap and it removes confounds from every later decision.
- **Scope:**
  - derive primary ray extents from the actual GL-style clip range (z = −1 to just inside the far plane, or near/far taken from the projection);
  - verify and correct the D3D12 W convention alongside ADR-004 fog;
  - add the per-frame population counter series (§7.1);
  - optionally, fix the FullSync metadata loss by re-applying the per-task latch after `begin()`.
- **Systems:** `PrimaryHitRT.hlsl`, `RasterPS.hlsl`/CMake shader flags, framebuffer renderer instrumentation aggregation.
- **Evidence required:**
  - held Town/Termina captures showing guide coverage before and after at >1.7k units;
  - one D3D12 frame;
  - one reproducing camera-pan series.
- **Decision first:** none beyond confirming that expanded coverage is wanted (it is the documented intent).
- **Out of scope:** changing culling, persistent geometry, projection-selection policy.

### WP2 — Environment-source responsibility and a frame-level source collection *(one focused design pass, then implementation)*

- **Problem:** F3, F4 and the source half of F7.
- **Why now:** it fixes the night-shadow class semantically and unblocks stable local GI and source presentation. It is also the prerequisite for any budgeted many-source transport.
- **Scope:**
  - **Adapter side:**
    - publish per-directional role, visibility authority and permission, derived from MM's own shadow semantics and light mode;
    - publish a `LightContext`-derived source set with stable IDs (for example `LightNode` lifetime) independent of binding receipts.
  - **Receipts:** they keep their exact ownership role.
  - **Renderer side:** owns selection and budget from that set, with value-dedup removed.
- **Decisions first:**
  - the source-ID lifetime rule;
  - whether an addition without visibility defaults to "no addition" (recommended);
  - an N-directional record or keeping two;
  - how bound (owned) and published (available) instances of the same source reconcile, to avoid double lighting.
- **Out of scope:** new energy curves, emissive geometry, source presentation itself.

### WP3 — RT scene record with temporal identity *(focused architecture/design pass first — highest leverage)*

- **Problem:** F5, plus the identity half of F8.
- **Why now:** it is the dependency for BLAS reuse, motion, history, persistent transport and any caster-lifetime policy.
- **Scope:**
  - a renderer-owned per-Workload scene built once and refit per HFR occurrence;
  - surface identity keyed through stock matching/transform groups;
  - per-pixel motion from stock previous transforms and velocity, taken per *display occurrence* (not per game frame);
  - a frame seed;
  - resource keying by framebuffer identity, not ordinal;
  - an RT+ requirement flag separate from stock `raytracingEnabled`.
- **Evidence required:** WP1's population series, which decides whether membership persistence is even needed; before/after AS cost at HFR.
- **Decisions first:**
  - identity granularity (transform group × call hash vs RDP call);
  - behaviour when matching fails (no history, never fabricated identity);
  - whether a scene can span framebuffers or projections.
- **Out of scope:** persistent off-screen or game-culled geometry until evidence demands it; choosing a denoiser.

### WP4 — Reconstruction seam and first temporal backend *(implementation after WP3)*

- **Problem:** F8.
- **Scope:**
  - a backend interface with a canonical input set (raw signal, depth/normal, stable ID, motion, previous guides, sample index, confidence out);
  - history validity and disocclusion rejection;
  - call-identity rejection in the spatial fallback;
  - one custom temporal accumulator, using individual algorithms only.
- **Evidence required:** the GI flicker split (§7.2), and held versus live A/B.
- **Decision first:** which responsibilities may use history (GI and fill first; direct visibility probably not).
- **Out of scope:** vendor SDKs, joint upscaling, PBR input invention.

### WP5 — Semantic frame packet and RT+ configuration ownership *(design pass together with WP2, since WP2 adds fields)*

- **Problem:** F9, F10 and part of F11.
- **Scope:**
  - carry frame-global semantics *with the task or display list* (a GBI command referencing a versioned packet, or a task-tagged submission), revisiting ADR-002's transport choice;
  - clear `valid` outside Play;
  - separate lighting semantics from atmosphere;
  - move RT+ settings into an RT64-owned RT+ configuration with an explicit precedence stack (§10) and a single defaults source;
  - consolidate MM profile values in the adapter;
  - version the atmosphere mod ABI.
- **Decisions first:**
  - GBI vs task-tag transport;
  - which layer owns defaults;
  - the precedence order;
  - which fields are semantic (adapter-only) versus configurable.
- **Out of scope:** a plugin ABI, a multi-game profile registry.

---

## 10. Configuration and profile layering (responsibility, not schema)

Precedence, low to high:

1. Stock RT64 defaults.
2. Optional RT+ renderer defaults (RT64-owned).
3. Optional per-game RT+ profile (data shipped with the recomp).
4. User configuration.
5. Session/developer overrides (launch, F1). These are never overwritten by a Graphics apply of unrelated fields.
6. The F9 effective master override, applied last without rewriting anything below it.

Orthogonal to that stack:

- **Semantic publication** comes from the adapter at runtime and is the *only* source of truth for what exists and what it means: sources, roles, directional validity, visibility authority derived from game rules, receiver provenance.
- **Adapter permissions** (for example "primary may influence unowned receivers") are semantic assertions. A profile may *scale or deny* them, but never grant them where the adapter did not publish them.
- **Explicit art-direction overrides** (like ADR-005's masked event) are published per frame by game code or mods, apply above the profile, and remain field-masked.

**What could work without recomp modifications:**
- per-pixel RSP lighting;
- contact AO;
- receiver classification;
- generic visibility against RSP directionals, if a profile declares an interpretation of observed RSP state (for example "treat world-draw directional slot 0 as environment primary"). This is a declared interpretation of runtime data, analogous to slot matching, not invented semantics.

**What needs publication from the game:** sources with position and radius, source identity, environment roles, visibility authority, emissive authorization, source presentation.

---

## 11. Answers to the decision questions

1. **Stop reconsidering:**
   - the responsibility and ownership-vs-permission model;
   - per-responsibility fallback, with Native as the reference;
   - receiver rejection instead of darkening;
   - one shared RT scene and query convention;
   - no appearance-based emission;
   - `SemanticLight` as a slot-independent record;
   - the ADR-012 energy-reference interpretation;
   - the F9 effective-override shape;
   - Plume as-is.
2. **Products of incremental evolution to consolidate now:**
   - scene lifetime and identity (F5);
   - source collection (F3);
   - environment-directional responsibility record (F4);
   - implicit ABI requirement bits and overloaded fields (F6);
   - the semantic packet transport (F9);
   - configuration ownership (F10).
3. **Right stock abstractions?** Yes for Workload replay, GPU RSP, Extended GBI, Plume and the HFR geometry path. Temporal identity and configuration are the exceptions.
4. **Duplicated or bypassed stock functionality:**
   - frame matching, transform groups, prev transforms and `worldVelBuffer` (unused);
   - `UserConfiguration`/`EnhancementConfiguration`/`GameConfiguration` (bypassed);
   - the `raytracingEnabled` flag (repurposed).
5. **Invariants vs historical limits:**
   - *Invariants:* slot ownership for replacement, authored hue and zero energy, SHADE ≠ albedo, glow ≠ light, Native reference.
   - *Historical limits:* source availability through binding, scene membership through game culling, asymmetric treatment of the two environment directionals.
   - *Safety boundaries:* scalar caps and candidate budgets.
6. **Compute spent recreating a limitation:**
   - full AS rebuilds for every HFR occurrence of identical topology;
   - shadow rays toward a below-horizon fill;
   - per-frame matching/VertexProcessor forced by a repurposed flag.
7. **Suitable for additional recomps?** The generic renderer side is suitable. The boundary is not yet: an unversioned, word-indexed host mailbox; two fixed directionals; configuration living in N64ModernRuntime; MM constants in `RasterPS`.
8. **Credible path to persistent geometry, temporal reconstruction and performance?** Yes, but only after a scene record with stock-derived identity exists. None exists today.
9. **Can reconstruction and transport remain replaceable?** In data, yes. In code, not yet (no seam), and not without motion and identity. Game semantics and authority do not need to change for it, which is the important part.
10. **Anything blocking richer ray/path-traced rendering?** Nothing is irreversible. The binding-defined source set and the light-sum GI tint are the two things path transport would have to replace first.
11. **Can an RT+ profile improve portability?** Yes, if it only scales or denies adapter-published permissions and supplies defaults. It must never declare sources, roles or visibility authority.
12. **Single most valuable foundation:** a **renderer-owned RT scene record with cross-frame identity**, covering surfaces through stock matching and sources through adapter-published IDs, per Workload and refit per HFR occurrence. It first needs WP1's coverage fixes and population evidence, so that its membership policy is chosen from data rather than assumed.
