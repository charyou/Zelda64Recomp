# A lighting vision for low-poly Majora's Mask

**Design proposal, 2026-09-10.** Run 2 is complete; this document implements nothing. It uses only facts established during that run, the user's observations, and the six supplied screenshots. There was no additional repository investigation, external research or research delegation for this synthesis. Current implementation details remain in `RAYTRACING_FOUNDATION.md`; proposals below are not existing capabilities or accepted implementation contracts.

## Direction: give the authored world depth, not a new identity

The destination is a painted, low-poly world whose inhabitants belong in their surroundings. Sunlight should reveal large forms; open sky should gently shape their shaded sides; small sources should create local islands of color; contact should hold feet and objects to the ground even without direct light. None of this requires glossy stone, metallic-looking cloth, uniformly bright interiors, or physically exhaustive light transport.

The next large visual gain is unlikely to come from more elaborate sun shadows alone. The supplied views expose a broader gap: **direct visibility exists, but spatially informed fill and dependable grounding do not yet form a complete lighting model.** Adding stronger AO everywhere would replace flatness with dirt. Adding more ambient brightness would preserve flatness. We need a small amount of occlusion paired with restrained, directional environment response, retaining the game's authored brightness and palette as constraints.

## What is established, and what is only suggested

Established in Run 2:

- Hardware rays intersect participating real geometry. Primary hits retain replay-local geometry/call identity, primitive identity and barycentrics; the secondary path can return opaque visibility without a new renderer.
- Global sunlight direction comes from the published resolved MM environment vector. Its visibility now modulates compatible existing directional diffuse terms in Enhanced rendering. It does not shadow ambient or multiply the final framebuffer.
- Coverage remains conservative: one selected world projection, submitted opaque triangles, limited receiver/light compatibility, double-sided opaque casting. Native and unsupported raster content retain the original path. Actual local/positional shadow lighting, AO, sky fill, temporal filtering and GI were not added.
- The user verified the default-off Graphics/F1 control. Focused Vulkan evidence exists; broad renderer qualification does not.

User observations and screenshot evidence:

| Reference | What the image helps evaluate | What it cannot establish alone |
| --- | --- | --- |
| 1 — interior with characters, shelves and plant | Muted green/yellow authored mood; broad surfaces and characters have limited spatial separation. Preserve the room's dim character. | Whether a particular light or surface was classified, or why direct lighting is weak. |
| 2 — night Town, torch and hovering fairy-like source | Useful cool-background/warm-source contrast. The torch and small moving glow should not have equal lighting authority. | The sources' actual runtime light representation or whether visible darkness is baked into textures. |
| 3 — daylight wooden porch and Link | Strong daylight and wood texture already carry the scene. Foot/contact readability matters more than adding shine to every plank. | A matched measurement of current shadow strength or the exact receiver eligibility. |
| 4 — wide daylit Town | Large forms need readable light/fill separation without tracing every painted stone as relief. | Whether weak lighting is a sun-match, material, coverage or artistic issue. |
| 5 — green forest and blue-green atmosphere | Existing color atmosphere and depth layering are valuable. Grounding must preserve this mood rather than neutralize it. | The current contribution of any one renderer feature, despite the user's reported enabled settings. |
| 6 — enclosed/open-roof-looking wooded area | Strong geometric shadow shapes coexist with dark authored textures. This is a warning against emphasizing low-poly structure further. | Whether an opening should admit direct sunlight at this time, or the cause of a coverage gap. |

The six originals are preserved as `user-reference-1.png` through `user-reference-6.png` in the ignored `finished-run2` evidence directory. The user reports sunlight shadows and Atmospheric fog enabled. These are qualitative references, not controlled A/B tests.

Two unresolved reports must remain separate. Sunrise shadows briefly appear/disappear before stabilizing; the user's research into disagreement among CURRENT_TIME, skyboxTime, light/RGB transitions and sun elevation is a **plausible explanation, not a diagnosis**. Camera movement into/behind geometry caused another temporary shadow collapse; its cause is unknown. Neither should be hidden by an artistic fade or denoiser. Many interiors lacking meaningful direct light and some open-roof areas behaving unexpectedly are also observations, not one proven bug.

## Give each lighting term a specific job

### Authored appearance is the reference, not assumed clean albedo

Textures, vertex colors, ambient terms and existing lights together define the appearance. Some textures already contain shadow, highlights or color grading. Treating those pixels as physically unlit albedo and adding a full second lighting solution would double-light the world and exaggerate baked shading.

Retain the original combiner, alpha, fog and special-effect behavior. Preserve unsupported draws automatically. Enhancements should first replace or redistribute an identifiable component; otherwise use a small, explicitly budgeted addition. A surface becoming eligible must not abruptly change its average exposure or color. Keep Native and an Enhanced feature-off view as references.

### Direct sunlight: large-scale directional structure

Use the semantic environment source, current compatible authored diffuse contribution and RT visibility already established. Visibility answers whether participating geometry blocks that direction; it does not determine whether the sun is bright, whether the room has usable ambient light, or whether a surface ought to look wet.

Keep source strength, source color, visibility and confidence separate. A dark shadow should reveal the remaining fill, not force black. An intentionally lit room may have no compatible sun contribution; casting another sun ray will not invent the missing light. An open roof is an opportunity for environment access, not permission to reinterpret every existing directional light as the sun.

Eventually move from razor-hard edges to restrained source-sized softness. Preserve crisp contact and recognizable shadow silhouettes. Do not simply blur the final shaded image, or increase softness until low-sun stability problems become hard to notice.

### Local direct lights: small artistic territories

Local light ownership requires a trustworthy location, extent and contribution. Draw-local RSP directions alone do not establish those properties. Preserve their existing appearance when a positional/source association is unavailable; do not turn every resolved direction into a guessed point light. Future generic semantic inputs or mod opt-ins may supply the missing information without requiring existing mods to change.

Proposed categories are artistic defaults, not actor-ID lists:

| Category | Range, intensity and color | Proposed visibility and shadows |
| --- | --- | --- |
| Tiny companion/firefly/magic mote | Very short range, low added energy, restrained color; retain its visible glow. It should brush a nearby face or sleeve, not illuminate a whole courtyard. | Usually no cast shadow initially. If useful later, weak, soft, short-range visibility. Avoid a flickering moving searchlight. |
| Candle or small lamp | Small pool, gentle warm or authored tint; source itself may remain bright while surrounding light stays modest. | Soft local contact/shadow only when visually useful. Allocate few samples and avoid long dramatic silhouettes. |
| Torch, brazier or substantial lantern | Medium pool with readable falloff and stronger warm/cool contrast against the surroundings. Keep modulation slow and small enough not to pump exposure. | Stronger local occlusion with finite-source softness; limited casters/range. This is the first worthwhile local-shadow production target. |
| Large doorway, luminous opening or broad fixture | Broad, low-frequency direction and gradual falloff; principally a source of fill rather than a sharp spotlight. | Broad/soft visibility. Representing it as one hard point light is usually the wrong look. |
| Transient spell, hit flash or effect | Preserve authored timing and compositing; short-lived added light only with explicit energy/range. Saturated accents should not recolor the entire scene. | Normally unshadowed or very selectively shadowed. Avoid forcing expensive, unstable visibility on every particle. |
| Painted glow or decorative emission | Appearance only unless actual lighting semantics identify an emitter. | No automatic light or shadow allocation. |

Source size, persistence, authored intensity, supplied range and available semantics should choose behavior. Bright texture pixels are only a weak candidate cue. Cap simultaneous shadowed locals by projected relevance and influence, with stable selection and smooth handover. Never silently drop the original game's lighting to satisfy that budget.

### Contact grounding and RT AO: related, not two darkening passes

Contact grounding is a visual requirement: feet, crates and furniture should meet their support surfaces at noon, at night and indoors. It does not require a strong sun. Small-radius RT AO is a plausible first mechanism, using the current surface hit, normal and reusable secondary visibility path.

Use the near part of the AO distance response as the contact term. If a separate contact signal is needed, combine the two by a bounded envelope or shared visibility budget, not multiplication that produces double-dark corners. Contact should diminish as the object separates from the support; a jumping character should lose its tight footprint naturally.

AO measures nearby obstruction. It is not a new direct shadow, a generic darkness multiplier or a substitute for GI. Initially apply a restrained amount only to a separable ambient/environment component, with a protected brightness floor. Do not attenuate directly lit diffuse, emission, UI, fog or the final material color. Where that component cannot be identified safely, retain existing shading until a bounded bridge exists.

Retire legacy blobs only after the replacement is dependable in the relevant camera/state/material path. Suppress a blob only with a trustworthy association and validated replacement support; otherwise retain it. No global deletion based solely on an RT checkbox. Fade out overlap when replacing it, and restore fallback cleanly when support is lost. Do not claim that screen-visible AO covers an offscreen or unsupported character merely because RT is enabled.

### Sky/environment visibility and fill: the missing broad shape

Sky access is not identical to sun access. A courtyard can have visible sky but a blocked sun; an interior can receive light through an opening; a cave can retain stylized authored fill without an actual sky ray reaching infinity.

Propose a low-frequency environment response: a soft upper hemisphere, a muted lower/bounce hemisphere, and a small amount of directional bias from environment visibility. Modulate a replaceable share of authored ambient rather than stacking a second full ambient light. Preserve a protected authored floor in enclosed rooms. If no safe ambient partition exists, begin with a low-energy addition and prove that exposure remains stable before expanding it.

Longer-range sky access and short-range AO need distinct meanings and distance ranges. A TLAS miss is not proof of open sky: Run2 knows only submitted eligible geometry, so absent roofs or game-culled walls can create false openness. Sky fill must therefore wait for defensible coverage/confidence or use a conservative fallback. Do not build an arbitrarily huge ambient radius on the current incomplete scene and label every miss daylight.

Time/weather color and semantic environment state can inform the fill palette. Broad fill can give characters shape when no useful direct light exists, but it should not make dark rooms resemble overcast exteriors. Avoid assigning indoor/outdoor behavior from scene IDs; use available semantic context, actual geometry and honest fallback.

### Stylized indirect lighting, then subtle GI

A restrained environment/bounce model can deliver useful indirect appearance before traced GI: broad color influence from the sky, the authored room palette and a muted lower hemisphere. It should be described as an approximation, not claimed to be physical transport. It needs its own energy budget so direct, ambient and approximation terms do not all pay for the same light.

Later GI should replace or refine part of that approximation, not simply add another complete layer. Aim for subtle color connection under roofs and around large surfaces, not bright color bleeding from every texture. Low-frequency diffuse bounce is more valuable here than an expensive general path tracer. Keep emission-to-light conversion selective and prevent baked bright texels from becoming accidental emitters.

## Continuous time and weather direction

Use a coherent presentation state derived from existing environment semantics. Do not alter game time or recreate MM's original rules in RT64. The exact reconciliation of source data remains future work. A continuous artistic envelope should control light energy once the underlying semantic/visibility validity is understood; it must not conceal either known Run2 instability.

- **Daylight:** preserve authored sun color and broad direction. Sun/fill contrast should reveal forms while keeping shadowed characters readable. Contact remains small and present, not stronger merely because sunlight is strong.
- **Low sun and twilight:** change direct energy and warmth smoothly with resolved light state and sun elevation; retain sky fill after the direct sun loses authority. Give warm local sources room to become noticeable without instant intensity jumps. Do not darken the whole world at a single elevation threshold.
- **Night:** preserve the scene's authored palette and intended visibility. Moon-like directionality should require trustworthy semantics rather than assuming a reversed sun is always a moon. Environment fill supplies quiet shape; locals become prominent mainly through contrast, not automatic extreme boosts.
- **Cloud and storm:** reduce direct contrast continuously and broaden the remaining illumination. A cloudy sky may produce relatively important fill even while total brightness decreases. Shadow softness may increase with the artistic source model, but rain alone should not arbitrarily blur every shadow.
- **Weather color and atmosphere:** coordinate fill and direct tint with the existing mood. Atmospheric fog stays a participating depth/color term, not an exposure correction or a way to hide noisy AO. Wetness must not automatically turn all surfaces into mirrors; selective material response comes later.

Keep distinct controls for source radiance, visibility contrast, angular/source size, fill balance and color. Smooth authored transitions and light-budget handovers; reset or reject invalid temporal history separately. Avoid both binary sun/no-sun switching and permanently washed-out shadows. Rapid legitimate flashes remain effects, not reasons to smooth every environmental change indiscriminately.

## Coarse Surface Classification without a PBR conversion

Use a small generic surface record and independent response traits rather than a large mutually exclusive taxonomy. Start from actual rendering semantics and the retained draw/primitive identity. Classification confidence is part of the record; uncertainty should reduce added behavior, not generate confident guesses from an asset name.

Useful proposed traits:

- Opaque/alpha/special-compositing participation and confidence; caster/receiver permissions remain explicit.
- Lighting ownership: authored-color-only, compatible directional diffuse, supported local contribution, or unknown.
- Broad roughness/specular response: matte default, satin, selectively glossy. These are art controls until a specific shading model warrants numeric roughness; no promise of measured PBR values.
- Environment response strength and tint policy, bounded by existing appearance.
- Reflection eligibility: default off; stronger evidence required for water, mirrors or deliberate polished surfaces. A shiny-looking painted texel is insufficient.
- Emissive appearance versus actual local-light candidacy as separate properties.
- AO/contact response and thin/small-surface sensitivity, with conservative fallback for ambiguous foliage, particles and effects.

A few human-readable families can supply defaults: matte solid, soft/organic, satin solid, explicitly reflective/liquid, luminous appearance, and special/unknown. They should not become per-asset authoring requirements. Do not infer physically metallic behavior from gold-colored cloth or a shield texture. Start Run3 with only the traits needed for ambient/contact eligibility; add reflection/specular traits when a real consumer needs them. Future mods may supply richer hints, but vanilla and compatible mods should benefit automatically from semantics already available.

No existing Run2 field proves those material properties. Its call/index mapping is the extension point, not a completed classifier. Replay-local IDs are also not temporal object identity; later history needs a justified continuity mechanism.

## Low-poly safeguards for AO and environment response

Lighting must explain object relationships more strongly than tessellation.

1. Use geometric normals for robust ray offsets and hemisphere safety, but the authored/supported smooth normal basis for broad lighting response where appropriate. Never indiscriminately smooth across a real wall corner or erase intentionally flat faces.
2. Favor short-range, low-strength, distance-weighted occlusion. Reject or discount self-intersection and nearly coplanar self-occlusion rather than outlining every triangle. Bias should scale sensibly without detaching foot contact.
3. Separate tight contact from broad enclosure. Nearby independent surfaces justify a contact cue; a normal change along a coarse mesh alone does not justify a dark seam.
4. Keep the lowest supported ambient contribution readable. Already dark/baked textures need less added occlusion, but brightness alone is not reliable material classification. A global conservative cap is safer than guessed texture-by-texture compensation.
5. Filter visibility/AO in their own signal space with depth/surface-aware rejection. Do not blur the final image, spread occlusion across silhouettes or turn fine texture detail into geometric creases.
6. Keep sky/environment directional response broad. Bent-direction information should be low-frequency and confidence-weighted; tiny changes at triangle boundaries should not create alternating bright and dark facets.
7. Judge motion, feet, faces, sloped floors and wall junctions. A still image with dramatic dark creases is not evidence of successful grounding. Preserve subtle shadows on the porch and forest floor without engraving the mesh structure into them.

A noisy, over-dark first AO pass is a debug result, not a finished visual gain. If stable sampling cannot fit the run, ship it default off and checkpoint honestly rather than compensating with heavier blur or blacker contact.

## Temporal reconstruction and denoising are consumers

Keep raw visibility, short-range AO/contact, environment visibility, direct diffuse, indirect diffuse and any future specular contribution distinguishable. Keep linear signal conventions, validity/confidence, depth, useful normals, hit distance where meaningful, and eventual motion/history identity explicit. Add a field when its immediate consumer requires it; do not build a speculative universal G-buffer now.

A later denoiser or reconstruction layer may make sparse samples economical. It must not decide whether sunlight exists, invent missing surface eligibility, fix an incomplete caster set, or own the game's art direction. Disocclusion, invalid camera states and environment transitions need appropriate history rejection; otherwise stability can merely become persistent error.

AMD FSR Ray Regeneration is a possible future consumer, not a dependency or a verified integration plan. No current API, platform support or exact input requirements were researched here. Evaluate the then-available contract before selecting it. Any small vendor material-type classification should be an **adapter projection** from the project's richer response traits, not the authoritative Surface Classification. Do not reshape the entire renderer around a proprietary enum or assume a reconstruction feature supplies missing material truth.

## Intended final conceptual lighting model

At a supported shading point, the conceptual budget is:

```
protected authored appearance
+ spatially shaped, bounded authored/environment fill
+ existing compatible sun diffuse × sun visibility
+ supported authored/local diffuse × each source's visibility policy
+ restrained indirect replacement/refinement
+ rare, explicitly eligible specular/reflection response
→ original material/combiner behavior → existing fog/blending/presentation
```

This is a responsibility diagram, not a literal formula for adding raw framebuffer colors. The authored appearance already contains some of these terms. Implementation must identify what is retained, replaced or supplemented before summing. Near contact/AO shapes only the permitted fill/indirect budget; it is not another multiplier over the diagram. Unsupported content continues through the original path.

## Main visual rules

- Preserve painted texture, palette, silhouette and mood. Prefer readable forms over physical completeness.
- Ground characters in every lighting state; do not make that depend on the sun being active.
- Pair modest occlusion with thoughtful fill. Neither dirty corners nor uniform brightness solves flatness.
- Keep emission, direct light, ambient/fill and fog distinct. A glow is not automatically a room light.
- Make time/weather continuous art inputs, with validated semantics and no abrupt day/night exposure switch.
- Spend local-shadow authority on meaningful persistent sources; tiny moving companions remain subtle.
- Keep specular selective and mostly rough. Avoid blanket wetness, reflective floors and metallic-looking organic surfaces.
- Preserve fallback until a replacement has actually earned ownership, especially for legacy blobs and incomplete geometry coverage.

## Pragmatic future runs under constrained compute

Each run should finish one observable result, save a working binary and evidence, and checkpoint before optional groundwork. Root owns reasoning and quick computer-use checks. Reuse the existing finite workflow and the supplied day/interior/night references; no repeated archaeology or broad qualification by default.

| Proposed run | Mandatory visual / architectural result | Groundwork only when essentially free |
| --- | --- | --- |
| **3A — bounded stability pass** | Reproduce and separate the sunrise and camera/view failures with one targeted setup each. Correct a demonstrated bounded defect or record the exact unresolved boundary. Keep raw visibility and sun/light-match validity distinguishable. Do not spend the run on another broad MM research project. | Retain a validity/rejection reason if the touched dispatch/receiver code already exposes it. No new general diagnostics framework. |
| **3B — grounding and minimal Surface Classification** | A restrained short-range RT contact/AO result on the current opaque subset, affecting only a defensible ambient/fill budget. Visibly ground characters/objects in one day, one dim/interior and one no-direct-light state. If a safe ambient bridge cannot be bounded, complete that bridge and checkpoint rather than shipping framebuffer AO. Keep blobs until replacement eligibility is proven. | Only the classification fields this consumer needs; cheap normal/hit metadata already at the hit. No full material model, reflection database or scene taxonomy. |
| **4 — spatial fill and environment continuity** | Improve broad shape and shaded-character readability using a small, exposure-preserving environment/ambient rebalance. Establish trustworthy sky-access confidence before long rays control indoor brightness. Use continuous time/weather color and sun/fill balance; retain dim interiors. | Broad sky direction or enclosure signal if available from the same queries. No GI requirement and no promise to solve every open-roof case. |
| **5 — sample stability and restrained soft sun** | When sparse sampling limits the previous results, add the smallest justified history/reconstruction layer with disocclusion handling, then modest source-size shadows. Compare one supported denoising route against a simple baseline; choose by integration cost and visible benefit. Move this run immediately after3B if noise prevents a usable result. | Preserve separate signal exports useful to a later vendor adapter. No simultaneous multi-vendor framework or mandatory Ray Regeneration integration. |
| **6 — one meaningful local-light case** | Support trustworthy local-light position/range and demonstrate a torch/brazier pool with controlled visibility. Preserve existing light contribution and fallback. Tiny companion sources receive a deliberately weaker policy, not the torch settings. If supplied semantics are missing, test the smallest bridge before renderer expansion. | Stable source-budget selection or extra category defaults already exposed by that bridge. No particle-to-light classifier or all-scenes source extraction. |
| **7 — safe blob retirement and selective response** | Retire blobs only for reliably grounded supported cases. Add at most one high-value surface response—such as a deliberately reflective liquid or restrained satin highlight—if current classification has evidence for it. This run may be split; neither task justifies rewriting the material system. | Cheap response traits for an immediate consumer. Avoid broadly authoring or inferring per-asset materials. |
| **8+ — subtle indirect transport** | Add a bounded diffuse bounce that replaces part of the earlier stylized indirect approximation, with an obvious visual gain in a roofed/interior case and stable energy. Keep GI off until it improves the existing look. | A later reconstruction adapter or selective reflection refinement only if compatible signals and a concrete benefit already exist. |

This deliberately puts grounding and spatial fill ahead of general reflections or GI. Run3A is a small reliability gate, not an invitation to consume the AO budget with exhaustive sunrise archaeology. End it with a concrete fix or precise evidence and preserve the working subset. The original Run3 Surface Classification + AO intent remains in3B; classification grows with consumers rather than preceding them as a separate research program.

The successful endpoint is recognizably Majora's Mask: brighter where the environment explains brightness, quieter where the mood calls for darkness, and consistently connected at the feet and nearby surfaces. The renderer should make the original art feel more present, not replace its visual language.
