# Astra addendum — MM lighting facts + RT+ constraints

Use this as supplied analysis for the current GI/spatial-lighting continuation. Do **not** re-research these points unless the current working tree intentionally supersedes them.

## Established MM source facts

Research authority: `zeldaret/mm`, commit `486055ebec4c45f3020bfaa5d040698d6c2b85e4`.

### Global environment lighting
Primary source: `src/code/z_kankyo.c`
Relevant functions: `Environment_UpdateTime`, `Environment_UpdateSun`, `Environment_UpdateLights`.

In `LIGHT_MODE_TIME`, the global directional pair is derived from `CURRENT_TIME`:

```c
temp_s0_2 = var_v0 - CLOCK_TIME(12, 0);
light1Dir[0] = -(Math_SinS(temp_s0_2) * 120.0f);
light1Dir[1] =  Math_CosS(temp_s0_2) * 120.0f;
light1Dir[2] =  Math_CosS(temp_s0_2) * 20.0f;
light2Dir = -light1Dir;
```

`dirLight1` is the primary environment directional and the best **sun-like** source in ordinary time-based environments. `dirLight2` is the opposing global directional in time mode and remains independently colored. In fixed-light scenes, either direction may be authored and must not be assumed to be literal sunlight.

`Environment_UpdateSun` uses the same ordinary celestial orbit for `sunPos`, but visual sun state and lighting direction are separate semantics.

Resolved production values are:

```text
lightSettings.ambientColor + adjLightSettings.ambientColor
    -> play->lightCtx.ambientColor

lightSettings.light1Color + adjLightSettings.light1Color
    -> envCtx->dirLight1.params.dir.color

lightSettings.light2Color + adjLightSettings.light2Color
    -> envCtx->dirLight2.params.dir.color
```

Use the **post-adjusted directional RGB** as authored direct-light energy. MM has no canonical scalar `sunIntensity`.

At sunrise, `CURRENT_TIME` and `skyboxTime` can temporarily diverge. Direction follows `CURRENT_TIME`; ambient/direct RGB table selection follows `skyboxTime`. Therefore nonzero direction, `isNight == false`, visible sky or `sunDisabled == false` alone are not valid direct-sun-strength tests.

Useful original behavioral analogue: `src/code/z_actor.c::ActorShadow_DrawFeet`. A directional shadow candidate requires positive vertical direction and is weighted approximately by:

```text
(R + G + B) * abs(direction.y)
```

So meaningful sun/direct authority should consider both authored RGB and elevation, not direction validity alone.

### Authored local lights
Primary source: `src/code/z_lights.c`
Relevant APIs:

```text
Lights_PointNoGlowSetInfo
Lights_PointGlowSetInfo
Lights_DirectionalSetInfo
LightContext_InsertLight / RemoveLight
Lights_BindAll
Lights_BindPoint
Lights_BindPointWithReference
```

`LightInfo` is the semantic source truth:
- point lights: world position, RGB, radius/influence semantics;
- directional lights: direction, RGB;
- `POINT_GLOW` and `POINT_NOGLOW` are both real lights; glow is additional visual presentation.

The original N64 `Lights` group has at most seven actual light slots. Treat that as a raster binding limit, **not** as proof that only seven authored scene lights exist.

`Lights_BindPoint` can realize a true positional light; `Lights_BindPointWithReference` can approximate the same authored point source as an object-relative directional light. The semantic source remains positional.

World and actors can both receive authored local lights even when their original realization differs.

Verified torch example: `Obj_Syokudai` creates a genuine `POINT_GLOW` light and updates its warm RGB/radius while burning. Position, color, radius/activation and flicker already exist in game semantics; do not infer a second light from the bright flame geometry.

Bright-looking textures/sprites/particles are **not** sufficient light-source authority. Lightning is a counterexample: it changes environment lighting rather than inserting a giant local point light.

## Current renderer/GI facts relevant to this run

- Outdoor Clock Town already has broad Enhanced/per-pixel coverage; missing outdoor coverage alone does not explain the weak final A/B.
- Current ambient/authored response is intentionally conservative through budgets/floors, so Legacy lighting retains substantial authority.
- Current RT sun visibility modifies an appropriate original directional contribution; it does not remove unrelated ambient, secondary directional or local lighting.
- Current spatial-local response is intentionally bounded.
- Raw GI now visibly contains strong spatial structure.
- The documented in-progress GI bounce currently uses resolved environment + matching sun contribution, but **does not yet include local-light GI**.

Treat local-light GI as an identified missing source class, not an open research question.

## Required continuation

Finish the interrupted GI path from the current working tree: generation, bounce-hit lighting, reconstruction, production energy/composition, controls/diagnostics, build and runtime validation.

While doing so, design the concrete RT+ composition/authority model. Do not merely add a global ambient multiplier.

A useful user-facing abstraction may be:

```text
Current / Faithful  <---->  Enhanced / RT+
```

This should represent **lighting authority**, not framebuffer crossfade. Internally, ambient/indirect, primary sun-like direct, secondary directional and local direct may require different mappings.

Key constraints:
- Ambient/indirect is the main candidate for stronger Enhanced authority when receiver + reconstruction are trustworthy.
- Keep sun/direct separate from ambient; use MM direction + post-adjusted RGB + RT visibility, without inventing an arbitrary new sun.
- Keep semantic local replacement, unowned spatial-local direct and GI as separate ownership responsibilities; avoid double counting.
- Integrate verified authored local `LightInfo` sources into GI bounce lighting using a bounded source/ray budget where the current generic architecture permits it.
- Do **not** add generic `bright texture -> emitter` inference in this pass.
- Fallback is per lighting responsibility where practical: if Enhanced cannot safely own a responsibility, preserve the original contribution for that responsibility.
- Mixed Enhanced/fallback surfaces must not create obvious energy seams. Solve coherence generically; do not add Zelda actor IDs, scene IDs or texture-name hacks.

## Renderer-neutrality requirement

Keep the renderer game-agnostic.

Game-specific MM code should only **publish generic semantic data/capabilities** through the existing adapter/bridge layer. RT64 should consume generic concepts such as:

```text
ambient/environment RGB
environment directional[0..N]: direction + RGB + semantic/capability flags
semantic local sources: type + position/direction + RGB + range + authority/response flags
receiver traits / provenance
feature/profile capability bits
```

Do not put `Obj_Syokudai`, MM scene IDs, Zelda actor types, `CURRENT_TIME`, `skyboxTime`, or other MM symbols/branches inside generic RT64 shading logic.

The MM adapter may decide that a game-side source maps to e.g. `PrimaryEnvironmentDirectional`, `SecondaryEnvironmentDirectional`, `LocalPoint`, or a permitted spatial/GI source. The generic renderer decides how those semantics participate in Direct, GI and fallback.

Prefer profile/capability flags over game-name checks. A different N64 game should be able to provide the same generic semantic interface differently, or provide less information and fall back safely.
