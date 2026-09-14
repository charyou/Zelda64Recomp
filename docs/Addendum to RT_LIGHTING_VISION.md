# Post-Research Addendum to `RT_LIGHTING_VISION.md`

## Purpose and authority

`RT_LIGHTING_VISION.md` remains the primary design and artistic-direction document.

It was explicitly written without the later Majora’s Mask decompilation research and therefore could not incorporate the game-side semantics now documented in `MM_LIGHTING_SEMANTICS_RESEARCH.md`.

This addendum should therefore not prescribe a replacement architecture or roadmap.

Its purpose is narrower:

- confirm where Astra’s Vision is directly supported by the later MM source research;
- identify assumptions that now have stronger or more precise source evidence;
- identify places where Astra did not yet have enough information and should reassess its previous proposal;
- surface new opportunities revealed by MM’s actual semantics;
- leave final renderer architecture, feature ordering and implementation strategy to Astra.

Treat the MM research as authoritative for original-game lighting semantics, and Astra's judgment as authoritative for how those semantics should affect the modern renderer.

Repository reading note (2026-09-10): the design baseline is [RT_LIGHTING_VISION.md](RT_LIGHTING_VISION.md), whose post-research reading note integrates these qualifications. The named `MM_LIGHTING_SEMANTICS_RESEARCH.md` was not found in the inspected checkout; preserve the supplied findings here without presenting this addendum as a substitute for the full source report or claiming fresh source verification. Do not re-research established MM sunrise/time/environment behavior unless a specific renderer issue materially requires it.

---

# 1. Areas where Astra’s Vision is strongly confirmed

## Authored appearance should remain the reference

Astra’s decision not to reinterpret Majora’s Mask as clean PBR material data remains strongly justified.

The Vision correctly treats textures, vertex colors, existing ambient/directional lighting and the original combiner as part of the authored appearance rather than assuming the texture is physically clean albedo.

Nothing in the MM source research contradicts this. On the contrary, MM’s environment-light system reinforces how strongly color, ambient, directional lighting, fog and environment transitions are authored together.

The Vision’s general rule remains appropriate:

> modern enhancements should add spatial depth while preserving MM’s palette, mood and authored appearance rather than replacing them with a new physically based identity.

## Direct-light visibility should remain separate from ambient/fill

Astra was correct to give direct lighting and environment/fill different responsibilities.

Run 2 already follows this principle correctly: hardware visibility attenuates only the compatible RSP directional diffuse contribution. Ambient, fog, textures, combiner behavior and final framebuffer color are not multiplied by the shadow mask.

The MM research strongly supports preserving that distinction because ambient RGB and directional RGB are separate authored values and can evolve differently.

The existing conceptual separation between:

- direct source contribution;
- RT visibility;
- environment/fill;
- contact/grounding;

should therefore remain.

## Sky access is not sunlight access

Astra’s distinction between sky/environment access and direct sunlight is strongly confirmed by MM.

The source research establishes that MM does not have a single indoor/outdoor or sunlight Boolean. Skybox configuration, sun presentation, environment-light mode, room behavior, light-setting selection and positional-light support are separate pieces of state.

Collision surfaces can additionally request a different lighting configuration and transition to it over 20 frames, independently of apparent visual openness.

This directly supports Astra’s warning that an open roof or visible sky cannot automatically imply ordinary sunlight.

## Grounding should not depend on the sun

This is one of the strongest source-level confirmations of Astra’s Vision.

Astra argued that character/object grounding remains necessary at noon, at night and indoors and should therefore be conceptually separate from sunlight.

MM itself makes essentially the same distinction.

Generic `ActorShadow_Draw` shadows primarily provide projected floor grounding and do not derive their direction from the environment sun.

`ActorShadow_DrawFeet`, however, is genuinely light-aware and evaluates actual bound lights, including their direction and effective strength.

So Astra’s conceptual separation between:

**grounding/contact**

and

**directional light shadows**

is not merely a modern artistic preference. MM’s original renderer already separates those responsibilities.

## Local lights should not be inferred from bright pixels

Astra was also correct to treat emissive appearance and actual light ownership separately.

The MM research establishes explicit `LightInfo` objects for point and directional sources. A bright texture, flame sprite or particle is not by itself evidence of an actual renderer light.

This strongly confirms the Vision’s rejection of generic “bright pixel = light source” behavior.

## AO should remain restrained and should not become generic darkness

Nothing in the MM research undermines Astra’s AO/contact philosophy.

The Vision correctly treats AO as a short-range relationship signal rather than a new global shadowing model, and explicitly warns against multiplying it over direct light, emission, fog or final framebuffer color.

Given MM’s already-authored ambient/directional balance, this remains the safer artistic direction.

## Surface Classification should remain coarse and non-PBR

The later research does not reveal any reason to replace Astra’s coarse response-trait model with a traditional PBR material system.

Astra’s proposal to make Surface Classification grow only when real consumers require new traits remains appropriate.

The important refinement is only that game-side lighting semantics should not be inferred by the material classifier when MM can expose them more directly.

---

# 2. New MM semantics Astra could not have known

These are not errors in the original Vision.

They are new facts that should now be available to Astra when it reassesses the design.

## There is no single MM “sunlight state”

The research establishes that several quantities which might casually be called “sunlight” are separate.

In time-based lighting:

- global environment direction follows `CURRENT_TIME`;
- ambient and directional RGB interpolation is selected using `skyboxTime`;
- the visual `sunPos` is separately stored and updated;
- `isNight` changes independently;
- environment configuration transitions can occur independently;
- weather can affect RGB and visual sun state through separate paths.

Around 06:00, `CURRENT_TIME` and `skyboxTime` can deliberately diverge for a short interval. This means the primary direction may already be above the horizon while environment RGB is still being selected from the pre-06:00 side.

The research therefore establishes that:

> a valid environment/sun direction is not itself evidence that strong direct sunlight should already exist.

Astra’s Vision already anticipated a distinction between source strength, source color, visibility and confidence. The research now gives concrete MM state with which Astra can reassess that model.

## Sunrise and sunset are not one synchronized transition

MM’s dawn/evening behavior is more complex than a single continuous daylight curve.

At sunrise:

- celestial direction reaches the horizon at approximately 06:00;
- `isNight` switches immediately;
- `skyboxTime` can remain temporarily pre-06:00;
- the time-light setting pair changes at 06:00;
- visual-sun appearance also has its own altitude-dependent state.

At sunset, the timing differs again: lighting transitions already run through 17:00–19:00 while `isNight` changes at 18:00.

This should become background knowledge for future sunlight work.

It does **not** imply that Run 3 should be spent exhaustively reproducing or debugging every sunrise transition.

The observed transient remains worth documenting, but the new semantics should simply prevent future work from relying on oversimplified assumptions such as:

`valid sun vector = sunlight fully active`

or:

`isNight == false = strong sunlight`.

If later implementation naturally resolves the transient, that is sufficient. Dedicated investigation should only be necessary if the issue remains materially blocking.

---

# 3. Enhanced sunlight energy: an opportunity, not a source-code requirement

The MM research establishes that post-adjustment directional RGB is the best source-level representation of the original global direct-light contribution.

It already contains authored environment changes such as weather adjustments.

However, this does not establish that those 0–255 N64 values should be treated as final modern physical radiance.

MM’s lighting model was designed for the original hardware and shading pipeline.

The research therefore gives Astra two things:

1. a trustworthy **authored lighting state**;
2. freedom to decide how Enhanced rendering should translate that state into a richer modern lighting response.

One potentially valuable direction is a restrained modern sunlight-energy model which takes MM’s authored directional color/state and combines it with additional semantic information such as sun elevation and environment phase.

For example, Enhanced rendering could potentially interpret:

- very low positive sun elevation as warm but weak direct sunlight;
- higher daytime elevation as stronger direct authority;
- sunset as declining direct energy while environment/local light remains important;
- weather-adjusted MM RGB as a cue that direct illumination should be less dominant.

This should be treated as an artistic opportunity for Astra to assess rather than a prescribed formula.

A particularly important distinction is:

**RT visibility should continue to answer geometric obstruction.**

If dawn shadows are visually weak, the cleaner modern interpretation may be that only a weak direct contribution exists to be removed, rather than artificially making the shadow mask itself translucent.

The research also provides an interesting source precedent: MM’s own light-aware foot-shadow path considers both directional-light RGB magnitude and positive vertical direction when determining meaningful projected shadows.

This does not prescribe the Enhanced energy curve, but it supports treating source energy and elevation as distinct meaningful inputs.

Astra should decide how far the Enhanced renderer should reinterpret original N64 lighting magnitude while preserving the authored palette and dramatic intent.

---

# 4. Primary environment directional light versus visual sun

Run 2’s current narrow implementation uses the published MM environment/sun vector and matches actual compatible RSP directional lights against it. That was a reasonable Run-2 foundation.

The later research establishes additional distinctions Astra did not previously have:

- in ordinary time mode, the primary environment direction and visual sun normally follow the same celestial orbit;
- their downstream semantics remain separate;
- fixed/non-time environments can use arbitrary authored directional vectors unrelated to literal sunlight;
- cutscene visual-sun behavior also follows its own state path.

The implication is not necessarily that Run 2’s implementation should immediately be redesigned.

Rather, Astra should reassess whether the long-term conceptual model should distinguish:

**primary global environment directional light**

from:

**visual/celestial sun state**.

This distinction could be important for fixed-light scenes, interiors, cutscenes and future Enhanced sunlight-energy behavior.

The exact renderer interface should be Astra’s decision.

---

# 5. Indoor/outdoor semantics need explicit reassessment

This is important enough to call out separately.

The MM research establishes that there is **no single authoritative `isIndoor` or `isOutdoor` lighting flag**.

Instead, relevant game state includes combinations of:

- `lightMode`;
- active environment-light settings/configuration;
- `sunDisabled`;
- skybox configuration;
- room behavior;
- collision-selected light-setting/configuration;
- actual post-adjustment ambient/directional RGB.

A room can therefore have nonzero ambient and directional lighting with no visible sun or conventional skybox, and fixed-light mode may use authored directional vectors unrelated to the celestial sun.

Collision-driven lighting zones make this even more important: a visually open floor/room region can intentionally request a different environment-light configuration.

This confirms Astra’s caution around geometric sky inference, but also supplies richer game-side evidence that was unavailable when the Vision was written.

Astra should therefore reassess how much responsibility should belong to:

- MM’s environment semantics;
- geometric sky/openness queries;
- any existing or future indoor/outdoor classification.

No specific hierarchy is prescribed here.

The important constraint is simply:

> apparent geometry alone is not sufficient evidence for MM’s intended environment-light state.

Similarly, a TLAS miss is not automatically “open sky,” especially while the current RT scene contains only submitted participating geometry rather than guaranteed complete scene geometry.

---

# 6. Local lights are a larger opportunity than the original Vision could establish

The original Vision was appropriately conservative about local-light ownership because it did not know whether trustworthy positions, ranges or source identities existed.

The MM research now establishes that they often do.

`LightContext` contains semantic point and directional `LightInfo` objects. Point lights carry world position, RGB and radius.

MM can realize those point lights either as actual positional microcode lighting or as an object-relative directional approximation depending on the rendering path.

Torches are particularly clear: a torch publishes a real point/glow light, including position, color, active radius and dynamic flicker.

Some fairy actor modes also genuinely participate in `LightContext`, although the source research does not justify assuming that every visible fairy state does so.

This removes an important uncertainty from the Vision.

Astra should reassess whether the existence of those authored source semantics changes:

- when local lights should enter the roadmap;
- whether source-level information should eventually be exposed to RT64;
- how modern local shadows should relate to the original bound lighting contribution;
- whether torches, fairies and other sources should receive different modern policies.

One important constraint from the research is that blindly adding every `LightContext` source as an additional modern light could double-light content, because the same authored source may already have been realized through original RSP lighting.

The precise relationship between authored source state and Enhanced replacement/refinement should therefore be designed deliberately by Astra.

---

# 7. Actor-shadow replacement can now be reasoned about more precisely

The Vision was correct to avoid deleting legacy shadows globally.

The MM research now identifies two materially different original responsibilities.

Generic projected/blob shadows are primarily grounding effects.

Light-aware foot shadows respond to actual bound lights and their directions/strengths.

This suggests that Astra should reassess whether they should eventually be replaced by different modern mechanisms rather than treating “legacy actor shadows” as one feature family.

Potentially:

- contact/AO or another grounding solution addresses the generic blob responsibility;
- direct/local RT shadows address the light-aware responsibility.

This is only a conceptual mapping suggested by the original MM behavior.

Astra should decide whether it is actually the best Enhanced implementation strategy.

The current conservative Vision rule remains sound: do not retire an original shadow path until its visual responsibility is reliably replaced.

---

# 8. Surface Classification remains useful, but new MM semantics reduce what it needs to infer

Astra’s coarse Surface Classification remains useful.

The research does not imply a need for a large material taxonomy or PBR conversion.

The new information does suggest a useful separation of concerns:

Surface Classification can continue to answer questions such as:

- caster/receiver eligibility;
- opaque/cutout/special behavior;
- thin/two-sided sensitivity;
- AO/contact response;
- broad environment-response policy;
- emissive appearance;
- reflection/specular eligibility;
- rough artistic response traits.

But several things no longer need to be inferred primarily from surface appearance:

- whether an actual authored local light exists;
- the current MM environment/light configuration;
- weather state;
- celestial/environment directional state;
- room/environment transitions.

MM already has stronger semantic evidence for those.

Astra should decide what interface boundary best preserves that distinction.

---

# 9. Roadmap implications: provide Astra the facts, not a replacement roadmap

The current Vision proposes:

- a bounded stability pass;
- grounding/AO;
- spatial environment fill;
- temporal reconstruction and soft sunlight;
- local lights;
- selective response;
- later indirect transport.

The MM research creates reasons to reassess portions of that sequence, especially because:

- local-light semantics are richer than previously known;
- sunlight state is more nuanced than previously known;
- indoor/outdoor cannot be reduced to geometric openness;
- actor shadows already separate grounding from directional-light response.

However, this addendum does **not** recommend replacing Astra’s roadmap with a predetermined Run-3/4/5 sequence.

Astra should reassess the sequence using the new evidence and the current renderer cost/benefit.

The only strong procedural recommendation is:

**do not spend an entire upcoming run rediscovering the MM sunrise semantics that are now already established by the source research.**

Keep the observed sunrise and camera/view transients documented as known issues. Address them when they block or intersect productive renderer work, rather than making them mandatory standalone investigations merely because they exist.

---

# 10. New game-side information Astra should consider exposing

The MM research establishes several distinct semantic quantities that did not exist in the information available when the Vision was written.

Astra should assess which of these deserve first-class renderer exposure and at what abstraction level:

- primary global environment direction;
- secondary global environment direction;
- post-adjustment RGB for both global directionals;
- post-adjustment ambient RGB;
- `CURRENT_TIME`;
- `skyboxTime`;
- environment `lightMode`;
- current/previous/next light-setting or light-configuration state;
- active transition/blend state;
- visual `sunPos`;
- visual-sun enable/presentation state;
- weather/precipitation/lightning state;
- room/environment metadata;
- collision-selected lighting zones;
- `enablePosLights` and related original realization semantics;
- semantic `LightContext` point/directional sources including position/direction, RGB and radius.

The research identifies these as genuinely separate parts of MM’s lighting model rather than one unified “sun” state.

It is up to Astra to determine whether they should be exposed individually, combined into higher-level renderer semantics, or consumed in some other form.

### Additional source-established sunrise semantics

The later MM research establishes more than a general possibility of unsynchronized sunrise state.

In time-based lighting, `CURRENT_TIME` drives the celestial/global-light direction while `skyboxTime` drives environment light-setting selection and RGB interpolation. Around 06:00, `skyboxTime` can deliberately remain on the pre-06:00 side until `CURRENT_TIME >= 06:00 + 0x10`, while the primary direction and `sunPos` have already crossed the horizon.

This is established original MM behavior, not an RT64 hypothesis.

A separate transition also occurs at 06:00 because the 04:00–06:00 and 06:00–08:00 time-light segments do not universally use index-continuous setting pairs. In addition, `isNight` flips exactly at 06:00 even though those other lighting states follow different transition rules.

Sunset is asymmetric: the researched code shows no corresponding 18:00 `skyboxTime` hold, while the authored evening light transition spans 17:00–19:00 and `isNight` changes at 18:00.

Collision-selected environment configurations add another independent transition domain: a lighting-zone change can initiate a 20-frame configuration blend while the time-of-day transition is occurring simultaneously.

These facts should be treated as known MM semantics in future renderer work. They should not require a dedicated Run-3 rediscovery/debugging exercise.

The existing observed Clock Town RT-shadow transient is strongly consistent with these semantics, but the source research alone does not establish that these mechanisms are the complete causal explanation for that specific runtime observation. If exact causality ever matters, correlate the relevant MM state at runtime rather than re-researching the underlying game behavior.

---

# Final assessment

The later MM research does **not** invalidate Astra’s Vision.

It actually confirms several of its most important artistic and architectural instincts:

- preserve authored appearance rather than replacing it with PBR;
- keep direct light, environment fill and contact grounding conceptually separate;
- do not equate sky access with sunlight;
- do not infer local lights merely from emissive appearance;
- ground characters independently of the sun;
- keep AO restrained;
- delay broad reflections/GI until more fundamental lighting relationships work;
- keep material classification lightweight and consumer-driven.

Where the research changes things is mainly in the amount of semantic information now available.

Astra could not previously know that MM already exposes:

- distinct celestial direction versus environment RGB timelines;
- the sunrise `CURRENT_TIME` / `skyboxTime` split;
- separate visual-sun state;
- collision-driven environment-light zones;
- real semantic point lights with position/RGB/radius;
- different original actor-shadow systems for grounding and light-aware projection.

Those findings should now be supplied as authoritative game-side evidence.

Astra should use them to reassess the Vision and implementation priorities, while retaining authority over:

- the exact renderer abstraction;
- how Enhanced sunlight energy should reinterpret original N64 lighting values;
- whether and how local-light source semantics enter RT64;
- how indoor/environment classification should work;
- which legacy shadow mechanisms should eventually be replaced;
- and the final Run 3 / Run 4 / later sequencing.

The most useful framing is therefore:

**Astra’s Vision remains the design baseline.  
The MM research removes several uncertainties beneath it.  
Use the newly established source semantics to refine the design where they materially change what is possible or what assumptions are safe.**
