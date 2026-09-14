# MM_LIGHTING_SEMANTICS_RESEARCH.md

**Repository authority:** `zeldaret/mm`, `main`, researched against commit `486055ebec4c45f3020bfaa5d040698d6c2b85e4`. The repository is the active Majora's Mask decompilation project; the analysis below treats game code and extracted scene data as authoritative and treats names/comments only as secondary evidence. citeturn0search1

**Primary files:** [`src/code/z_kankyo.c`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/src/code/z_kankyo.c), [`src/code/z_scene.c`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/src/code/z_scene.c), [`src/code/z_lights.c`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/src/code/z_lights.c), [`include/z64environment.h`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/include/z64environment.h), [`include/z64light.h`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/include/z64light.h), and [`src/code/z_actor.c`](https://github.com/zeldaret/mm/blob/486055ebec4c45f3020bfaa5d040698d6c2b85e4/src/code/z_actor.c).

## Executive conclusions

The most important finding for a modern Zelda64Recomp/RT64 lighting model is that **Majora's Mask does not have one unified scalar or state called “sunlight.”** Several related quantities evolve on overlapping but demonstrably different timelines.

**Established from zeldaret/mm source:** In time-based environment-light mode, the primary global directional vector is analytic and continuous with `CURRENT_TIME`, while ambient color and the two global directional-light colors are selected/interpolated through `sTimeBasedLightConfigs` using **`gSaveContext.skyboxTime`**, not `CURRENT_TIME`. `Environment_UpdateTime` deliberately allows those two time values to diverge briefly at exactly 06:00. fileciteturn72file0

This is particularly relevant to sunrise. From 04:00–06:00, each time-light configuration uses one pair of light settings; at 06:00 it changes to the next pair. Some configuration rows are **not endpoint-continuous** across that boundary. For example configuration 0 goes from `3 → 12` during 04:00–06:00, then abruptly starts `0 → 1` during 06:00–08:00. Configurations 1 and 3 likewise change to different setting families at 06:00, while several other configurations happen to be continuous. fileciteturn71file0

Even more importantly, immediately at sunrise `Environment_UpdateTime` contains this logic:

```c
if ((gSaveContext.skyboxTime >= CLOCK_TIME(6, 0)) ||
    (CURRENT_TIME < CLOCK_TIME(6, 0)) ||
    (CURRENT_TIME >= (CLOCK_TIME(6, 0) + 0x10))) {
    gSaveContext.skyboxTime = CURRENT_TIME;
}
```

Consequently, when `CURRENT_TIME` first crosses 06:00 while `skyboxTime` is still just below 06:00, `skyboxTime` can remain on the pre-sunrise side until `CURRENT_TIME >= 06:00 + 0x10`. The global light direction and visual `sunPos`, however, continue from `CURRENT_TIME`. fileciteturn72file0

That is a real original-game semantic split, not a renderer hypothesis.

**Strong inference from the source:** A renderer that derives “sun exists” from the existence, validity, or length of `dirLight1` will be conflating several concepts. The environment direction is deliberately kept valid even when direct-light RGB is dark, weather-attenuated, below/at the horizon, or the scene is not meant to display a sun. `Environment_UpdateLights` even replaces an all-zero direction with `{1,0,0}`, meaning “nonzero vector” is expressly not a semantic availability flag. fileciteturn73file0

**Established from zeldaret/mm source:** Rain changes actual lighting, not merely particle effects. In the relevant environment-light settings, `gWeatherMode == WEATHER_MODE_RAIN` reduces ambient RGB and both global directional RGB values; independently, current rain precipitation smoothly fades the rendered sun's primary alpha toward zero. fileciteturn72file0 fileciteturn73file0

**Strong inference from the source:** The best original-game analogue for “how credible/strong is a downward-casting direct source?” is not a Boolean day/night value. The light-aware legacy foot-shadow code itself considers only lights whose vertical direction component is positive and weights them using approximately:

`(R + G + B) * abs(direction.y)`

This supplies unusually strong source evidence that both **light color/energy and elevation** matter to meaningful projected direct-shadow behavior. fileciteturn21file0 fileciteturn21file2

**Established from zeldaret/mm source:** There is no single authoritative `indoor`/`outdoor` lighting Boolean. Scene/room commands separately control skybox configuration, sun disable state, environment-light mode, room behavior, positional-light support, and the environment-light-setting list. Collision surfaces can additionally select a light-setting index, producing local lighting zones independent of visual sky visibility. fileciteturn75file0 fileciteturn51file1

**Possible implication for a modern renderer:** Preserve the separation between global environment direction, actual post-adjustment directional RGB, ambient RGB, time/light-setting transition state, visual-sun visibility, and weather. Do not collapse them into one “sun active” flag.

## Global daylight and sunrise semantics

The core data flow is:

`scene/room header + EnvLightSettings[]`
→ `EnvironmentContext`
→ `Environment_UpdateTime`
→ `Environment_UpdateSun`
→ `Environment_UpdateLights`
→ `LightContext + dirLight1/dirLight2`
→ `Lights_BindAll`
→ N64 lighting state.

The update order in the main environment update is explicitly `Environment_UpdateRain`, time-based sequence processing, `Environment_UpdateTime`, **then `Environment_UpdateSun`, then `Environment_UpdateLights`**. Both therefore see the same frame's `CURRENT_TIME`; the key distinction is that `UpdateLights` uses `skyboxTime` for table selection whereas the directional orbit uses `CURRENT_TIME`. fileciteturn82file0

### Direction and visual sun

**Established from zeldaret/mm source:** In `LIGHT_MODE_TIME`, `Environment_UpdateLights` computes the primary direction analytically:

```c
temp_s0_2 = var_v0 - CLOCK_TIME(12, 0);

light1Dir[0] = -(Math_SinS(temp_s0_2) * 120.0f);
light1Dir[1] =  Math_CosS(temp_s0_2) * 120.0f;
light1Dir[2] =  Math_CosS(temp_s0_2) * 20.0f;

light2Dir = -light1Dir;
```

The only special case is upside-down scenes, where 12 hours are added first. fileciteturn72file0

`Environment_UpdateSun` computes `sunPos` from the same orbit, multiplied by `25`. Outside cutscenes:

```c
sunPos.x = -(sin(time - noon) * 120) * 25;
sunPos.y =  (cos(time - noon) * 120) * 25;
sunPos.z =  (cos(time - noon) * 20)  * 25;
```

Thus in ordinary time-based outdoor use, the visible sun position and primary environment direction have the same underlying celestial trajectory. They are nevertheless **separate stored quantities with separate downstream behavior**. fileciteturn73file0 fileciteturn74file0

At approximately 06:00 the primary direction is at the horizon: its Y component passes through zero. After 06:00, Y becomes positive; before 06:00 it is negative. This follows directly from the cosine orbit centered on noon. fileciteturn72file0

There is also a concrete divergence during cutscenes. `Environment_UpdateSun` smooth-steps visual `sunPos`, and the current decompilation records an original bug where the update intended for Z writes Y a second time. The environment directional light does not follow that smoothing path. Therefore **visual sun position is not universally interchangeable with the lighting vector**, even though their ordinary-time formulas correspond. fileciteturn74file0

### Direct-light RGB and ambient RGB

`EnvLightSettings` provides authored environment values including ambient RGB, two directional-light directions/RGB values, fog RGB/near/far data. The active values are held in `envCtx->lightSettings`; temporary/environment adjustments are accumulated in `envCtx->adjLightSettings`. `Environment_UpdateLights` ultimately clamps:

`lightSettings.ambientColor + adjLightSettings.ambientColor`
→ `lightCtx->ambientColor`

and:

`lightSettings.light1Color + adjLightSettings.light1Color`
→ `envCtx->dirLight1.params.dir.color`

with the analogous path for light 2. fileciteturn73file0

This distinction matters for RT extraction: **the post-adjustment `dirLight*.params.dir.color` values are closer to the actual game-side direct-light contribution than the raw scene-table entries.** They already include weather and other environment adjustments before raster rendering. fileciteturn73file0

There is no independent floating-point sunlight intensity. Intensity is encoded principally through RGB magnitude plus the geometric directional-light dot product performed by the rendering pipeline.

### Time-based table interpolation

The seven `sTimeBasedLightConfigs` rows divide a day into:

| Period | Table behavior |
|---|---|
| 00:00–04:00 | fixed night setting |
| 04:00–06:00 | night → dawn transition |
| 06:00–08:00 | sunrise → morning transition |
| 08:00–16:00 | fixed daytime setting |
| 16:00–17:00 | day → late-day transition |
| 17:00–19:00 | sunset → night transition |
| 19:00–24:00 | fixed night setting |

Each entry contains `startTime`, `endTime`, `lightSetting`, and `nextLightSetting`. `Environment_UpdateLights` finds the interval containing `gSaveContext.skyboxTime`, obtains a normalized interpolation weight through `Environment_LerpWeight`, then interpolates ambient RGB, light1 RGB, light2 RGB, fog RGB, fog near and Z-far. fileciteturn71file0 fileciteturn72file0

Crucially, **direction is not obtained from these interpolated table settings in time mode.** After interpolating ambient RGB, the routine overwrites `light1Dir` and `light2Dir` from `CURRENT_TIME`. It then continues interpolating light colors. fileciteturn72file0

This is the clearest source answer to the requested sunrise question:

> A completely valid, continuously changing sun/environment direction can exist while its associated RGB/ambient environment values are still being selected from a different temporal representation.

The two quantities do not share one authoritative interpolation clock.

### The special 06:00 split

The explicit `skyboxTime` condition in `Environment_UpdateTime` creates an especially important state around dawn. `CURRENT_TIME` normally advances every update. `skyboxTime` normally follows it, except during the small interval beginning at 06:00 where it can retain its pre-06:00 value until `CURRENT_TIME >= CLOCK_TIME(6,0) + 0x10`. fileciteturn72file0

The size `0x10` is about 0.000244 of a full 16-bit day, or about **21 in-game seconds of clock time**. The real-time duration depends on `R_TIME_SPEED`/scene time speed.

During that tiny interval:

**Established from source**

`CURRENT_TIME`: post-06:00 and advancing.

`dirLight1/dirLight2 direction`: post-06:00 celestial trajectory.

`sunPos`: post-06:00 celestial trajectory.

`gSaveContext.save.isNight`: switches to false at 06:00.

`skyboxTime`: can still represent the final pre-06:00 state.

`ambientColor/light1Color/light2Color`: selected from the interval identified by `skyboxTime`, therefore potentially still pre-sunrise. fileciteturn72file0

The binary `isNight` state itself is simply:

```c
if (time >= 18:00 || time < 06:00)
    isNight = true;
else
    isNight = false;
```

It does **not** control the environment RGB calculations shown in `Environment_UpdateLights`. Treating `isNight == false` as “strong sunlight available” would therefore introduce semantics the original global-light path does not have. fileciteturn72file0

### Dawn-setting boundary continuity

There is a second, separate sunrise issue: the light table itself changes segment at 06:00.

For configuration 0:

```text
04:00–06:00   3 → 12
06:00–08:00   0 → 1
```

For configuration 1:

```text
04:00–06:00   7 → 8
06:00–08:00   4 → 5
```

For configuration 2:

```text
04:00–06:00   11 → 8
06:00–08:00   8 → 9
```

Configuration 2 is index-continuous, but 0 and 1 are not. Configuration 3 similarly changes from `15 → 16` to `12 → 13`; configurations 4–6 connect through 16, 20 and 24 respectively. fileciteturn71file0

Whether different indices contain identical or deliberately matching RGB values is scene-data-dependent. The code alone therefore establishes a **potential discrete setting-identity boundary**, not necessarily a visible RGB jump in every scene.

The current repository's `Z2_TOWN` XML is only an extraction description pointing to the scene/room binary and display lists; it does not itself spell out Clock Town's environment-light records. fileciteturn54file0 fileciteturn55file0

Accordingly, I would not claim from the evidence gathered here that Clock Town specifically uses one of the non-continuous configuration rows without decoding its scene data. The engine-level mechanism capable of causing such a boundary is nevertheless established.

### Visual sun activity is a separate concern

`Environment_UpdateSun` has its own state:

`envCtx.sunDisabled`
controls whether the sun update path runs.

`envCtx.precipitation[PRECIP_RAIN_CUR]`
controls a smooth fade of `sSunPrimAlpha`: toward `0` while raining and toward `255` otherwise.

Sun altitude determines `sSunColor` and `sSunEnvAlpha`.

`sunPos` itself remains a valid celestial vector independently of the table-driven light RGB. fileciteturn73file0 fileciteturn74file0

The altitude calculation effectively derives:

```text
altitude = sunPos.y / 25 = light1Dir.y

sSunColor = clamp(altitude / 80, 0, 1)
sSunEnvAlpha = 255 - clamp(altitude / 80 * 255, 0, 255)
sSunScale = 12 + 2*sSunColor
```

So even the visual sun has a continuous altitude-dependent appearance transition separate from environment-light RGB interpolation. fileciteturn74file0

**Possible implication for the modern renderer:** `sunPos` or `dirLight1.dir` can be perfectly valid while actual direct-light RGB is still weak or transitional. A “valid vector = sun enabled” test is not faithful.

## Weather and room/environment selection

### Rain and storms

Rain is an actual lighting state.

`func_800F6CEC`, which is called while resolving time-based light entries, applies explicit changes for `gWeatherMode == WEATHER_MODE_RAIN` when the relevant setting indices are in the 4–7 family:

```c
ambient:
R -= 50
G -= 100
B -= 100

light1:
R -= 100
G -= 100
B -= 100

light2:
R -= 100
G -= 100
B -= 100
```

Fog is also shifted toward prescribed storm values. fileciteturn72file0

Thus **weaker directional lighting under rain is authored game semantics**, not a modern weather invention.

Rain simultaneously changes the visual sun through a separate path:

```c
if (PRECIP_RAIN_CUR != 0)
    sSunPrimAlpha -> 0
else
    sSunPrimAlpha -> 255
```

using `Math_SmoothStepToF`. fileciteturn73file0

This is particularly useful semantically: the game itself distinguishes “celestial direction exists” from “sun disc is visually visible.”

`En_Weather_Tag` controls rain precipitation and lightning states; storm entry can set `lightningState = LIGHTNING_ON` and rain maximum to 60, while leaving the storm requests through the environment system. fileciteturn57file4

Lightning is another example where lighting is not represented as a conventional local point light. `Environment_UpdateLightningStrike` modifies `adjLightSettings`, including large temporary ambient changes, rather than inserting a lamp-like `LightInfo`. fileciteturn58file0

**Established:** weather affects ambient, directional RGB, fog, visible-sun alpha, precipitation, lightning and environment presentation.

**Strong inference:** a renderer can defensibly use the game's post-weather directional RGB and rain state to weaken direct RT sun/shadow influence.

**Possible implication:** do not independently invent an unrelated cloud-opacity model when game-side post-adjusted light color and precipitation already provide source semantics.

### Scene and room lighting architecture

`Scene_CommandEnvLightSettings` loads the current authored array:

```c
envCtx.numLightSettings = cmd->lightSettingList.num;
envCtx.lightSettingsList = ...;
```

`Scene_CommandSkyboxSettings` independently sets:

```text
play->skyboxId
envCtx.skyboxConfig
envCtx.changeSkyboxNextConfig
envCtx.lightMode
```

`Scene_CommandSkyboxDisables` independently sets:

```text
envCtx.skyboxDisabled
envCtx.sunDisabled
```

These independent fields are important evidence that **skybox, sun rendering and environment-light computation are related presentation systems but are not one Boolean semantic category.** fileciteturn75file0

Room behavior additionally provides:

```text
curRoom.type
curRoom.environmentType
curRoom.lensMode
curRoom.enablePosLights
envCtx.stormState
```

from the room-behavior command. fileciteturn75file0

`RoomEnvironmentType` includes such classifications as `ROOM_ENV_DEFAULT`, `ROOM_ENV_COLD`, and `ROOM_ENV_WARM`; these are environment-effect classifications, not a robust indoor/outdoor model. `ROOM_TYPE_BOSS` is materially used to disable `Environment_AdjustLights`. fileciteturn76file0 fileciteturn59file0

### Lighting zones selected from collision

One of the most important non-obvious paths is collision metadata.

`SurfaceType_GetLightSettingIndex` extracts a **5-bit light-setting index from collision surface data**:

```c
return SurfaceType_GetData(colCtx, poly, bgId, 1) >> 6 & 0x1F;
```

fileciteturn51file1

Actors such as `En_Viewer`, and other gameplay paths, call:

```c
Environment_ChangeLightSetting(
    play,
    SurfaceType_GetLightSettingIndex(
        &play->colCtx,
        player->actor.floorPoly,
        player->actor.floorBgId));
```

fileciteturn51file3

The semantics of that requested index depend on `lightMode`.

In time-based mode, `Environment_ChangeLightSetting` starts a **20-frame transition between time-light configurations** by setting `changeLightNextConfig`, `changeDuration = 20`, and `changeLightTimer`. fileciteturn5file1

`Environment_UpdateLights` then computes the old time-of-day lighting and the new configuration's time-of-day lighting in parallel and blends the two using:

```c
var_fs3 =
    (changeDuration - changeLightTimer) /
    changeDuration;
```

fileciteturn72file0

Therefore a room, doorway, floor region, cutaway or partially open architectural area does not have to follow the scene's apparent physical sky exposure. Authored collision can request a different environment-light configuration.

This is probably the strongest engine-level explanation for why an “open roof” is not equivalent to “ordinary outside” in MM.

### Indoor/outdoor is not an explicit sunlight contract

**Established:** the code reviewed does not expose a canonical `isOutdoor` flag that determines global sunlight.

Instead, sunlight-like behavior emerges from a combination of:

`lightMode`,
active environment settings/configuration,
`sunDisabled`,
skybox configuration,
room behavior,
collision light-setting selection,
and actual post-adjustment directional RGB.

A room may therefore have:

nonzero ambient RGB,
nonzero directional RGB with an arbitrary authored direction,
no visible sun,
no normal skybox,
or a fixed-light mode unrelated to the celestial orbit.

In fixed/non-time light mode, `Environment_UpdateLights` obtains **both direction and color directly from the scene's `EnvLightSettings` entries** rather than overwriting directions with the solar trajectory. It can either take the selected entry directly or interpolate between `prevLightSetting` and `lightSetting` according to `lightBlend`. fileciteturn73file0

So the existence of a global directional light in an interior is not evidence that the source intends literal sunlight.

**Possible implication:** for RT semantics, “sun-casting outdoor environment” should be more restrictive than “there is a directional light.”

## Local and gameplay lights

The local-light system is substantially richer than a simple list of emissive objects.

### Core types

`src/code/z_lights.c` establishes three `LightInfo` types with common APIs:

```c
Lights_PointNoGlowSetInfo(...)
Lights_PointGlowSetInfo(...)
Lights_DirectionalSetInfo(...)
```

Point lights store world position, RGB and a signed radius. Directional lights store XYZ direction and RGB. `POINT_GLOW` and `POINT_NOGLOW` use the same basic lighting bind path; glow additionally supports the visual-glow machinery. fileciteturn78file0

`LightContext_InsertLight` allocates a `LightNode` from a fixed buffer and inserts it at the head of `lightCtx.listHead`. `LightContext_RemoveLight` removes it. The `LightInfo` itself remains owned by the scene/actor that created it. fileciteturn78file0

This is important for extraction: node lifetime and light-parameter lifetime are separate.

The N64 `Lights` group has at most seven actual light slots. `Lights_BindAll` traverses the context list and stops effectively when no free slots remain; recent head entries therefore have priority over older tail entries. fileciteturn78file0

### Two different point-light realization modes

MM contains both genuine point-light microcode support and an older object-relative approximation.

When positional lights are enabled, `Lights_BindPoint` writes a point light with authored XYZ/RGB. Its attenuation coefficient is derived from radius:

```c
kq = 4500000 / radius²
kq = clamp(kq, 20, 255)

kc = 8
kl = -1
```

The light is also roughly frustum/radius culled before occupying a light slot. fileciteturn78file0

When positional lights are not being used for that draw, `Lights_BindPointWithReference` instead converts the authored point light into an object-relative directional light. For a reference object inside the authored radius:

```text
distanceWeight = 1 - (distance / radius)²
RGB = authoredRGB * distanceWeight
direction = lightPosition - referencePosition
```

with direction scaled toward a length of roughly 120. fileciteturn78file0

Thus the authored semantic object is still a positional light, but the original raster renderer may approximate it differently for different render targets.

For world rendering, `z_play.c` enables real positional lights when `roomCtx.curRoom.enablePosLights` is set, then calls `Lights_BindAll(..., NULL, play)`. fileciteturn79file1

For actors, `z_actor.c` takes actor flags and `curRoom.enablePosLights` into account and may either use positional microcode lights or pass an actor-relative reference position to the legacy approximation. fileciteturn79file0

This answers an important RT question: **authored local lights can meaningfully affect both static/world geometry and actors, even though the original representation can differ between them.**

### Torches

`Obj_Syokudai` is a clean example of a genuine authored scene light.

Initialization creates a `POINT_GLOW` light at the torch position plus an authored glow height and inserts it into `LightContext`:

```c
Lights_PointGlowSetInfo(
    &this->lightInfo,
    x,
    y + GLOW_HEIGHT,
    z,
    255, 255, 180,
    -1);

this->lightNode =
    LightContext_InsertLight(...);
```

A negative radius initially makes it inactive for lighting. fileciteturn28file0

While burning, the torch varies its RGB every update:

```c
lightIntensity = Rand_ZeroOne() * 127 + 128;

Lights_PointSetColorAndRadius(
    &lightInfo,
    lightIntensity,
    lightIntensity * 0.7f,
    0,
    lightRadius);
```

fileciteturn28file0

So the torch's game-side semantics already provide essentially everything a physically richer renderer wants: world position, warm authored RGB, flicker, activation state and radius.

A modern renderer does not need to infer a light merely because flame polygons are bright.

### Fairies and glowing companions

`En_Elf`, the fairy actor family used for fairy/Tatl-style behavior, contains real `LightInfo`/light-node state and can drive a glow light. The source includes a `lightInfoGlow` path and an actor-updated glow radius rather than relying solely on billboard emissiveness.

**Established:** at least some `En_Elf` modes genuinely participate in `LightContext`.

**Limitation:** several controlling `fairyFlags` bits and mode functions remain semantically weakly named in the decompilation, so assigning the local-light behavior to every visual fairy state solely by symbol name would overstate the evidence. The safe conclusion is “fairy actor family supports real dynamic lighting,” not “every fairy sprite always produces a world light.”

### Not every bright effect is a renderer light

Lightning provides the clearest counterexample: its illumination is implemented through global environment adjustments, not by dropping a giant point light into `LightContext`. fileciteturn58file0

Likewise, an emissive texture, flame sprite or particle is not sufficient evidence of an actual game light. The authoritative test is whether gameplay code creates/inserts a `LightInfo`, changes environment-light settings, or otherwise binds explicit lighting state.

## Legacy actor shadows

MM contains materially different shadow semantics, and treating all legacy actor shadows as “fake sunlight” would be incorrect.

### Generic projected/blob shadows

`ActorShadow_Draw` requires `actor->floorPoly`. It checks the vertical distance between the actor and detected floor and projects a supplied shadow display list onto the actual receiver plane using the collision polygon:

```c
func_800C0094(
    actor->floorPoly,
    actor->world.pos.x,
    actor->floorHeight,
    actor->world.pos.z,
    &mtx);
```

fileciteturn62file0 fileciteturn65file0

The shadow's distance scaling is essentially:

```c
dy = clamp(dy, 0, 150);
shadowScale = 1 - dy / 350;
```

and alpha is multiplied by the same distance-derived scale. fileciteturn64file0 fileciteturn65file0

This generic path does **not** derive its orientation from the environment sun direction. Circle, square and horse variants supply different projected meshes but retain the grounding/decal character of the system.

**Established:** these shadows primarily provide receiver contact and actor grounding.

**Possible implication:** replacing them only with direct-sun RT shadows would remove a visual function that exists indoors, at night and in environments with no plausible solar source.

### Foot shadows are different: they are light-aware

`ActorShadow_DrawFeet` is substantially more sophisticated.

It checks receiver height beneath each foot, then examines the actual bound light set. The code separates:

```c
numLights = mapper->numLights - 2;
```

which indicates that the last two lights receive special treatment corresponding to the two global environment directional lights. fileciteturn21file0

For candidate lights it only draws a directional foot shadow if:

```c
light->l.dir[1] > 0
```

and derives a strength quantity:

```c
lightNum =
    (R + G + B) *
    ABS(light->l.dir[1]);
```

fileciteturn21file0

Local light contributions are accumulated first; the two global environment lights are then reduced according to the local-light contribution before additional foot shadows are drawn. fileciteturn21file2 fileciteturn21file3

This is exceptionally relevant to sunrise.

At exactly the celestial horizon, the time-based `light1Dir.y` is approximately zero, so the legacy light-aware foot-shadow path will not obtain meaningful projected-shadow strength from that primary environment light even though X/Z form a perfectly valid direction. As the sun climbs, positive Y and RGB jointly increase its ability to create that projected component.

The second global direction is the negation of the first, so when light 1 points from above, light 2 points from below and fails the `dir.y > 0` test, and vice versa.

**Strong inference:** the original game's most directional shadow-aware character path already encodes the principle:

> a valid celestial direction is not sufficient; the source must also be bright and geometrically above the receiver.

That is precisely why a binary “sun vector exists” interpretation is semantically unsafe.

At the same time, generic circular/blob shadows remain available as grounding effects independent of direct illumination. The legacy rendering therefore implicitly splits two responsibilities that a modern solution may eventually need to preserve: **directional illumination shadows** and **non-directional grounding/contact cues**.

## Renderer-facing semantic model

The following is the minimum game-semantic model I would expose to a future rendering agent. This is deliberately a semantic interface, not an RT64 implementation design.

| Semantic input | Authoritative source | Lifetime / update | Space / continuity | Important edge cases | Confidence |
|---|---|---|---|---|---|
| **Primary global environment direction** | `envCtx.dirLight1.params.dir`, produced by `Environment_UpdateLights` | Every environment update/frame | World-space direction; continuous in `LIGHT_MODE_TIME`, table/blend-derived in fixed mode | At/under horizon it is still valid; zero direction is forcibly changed to `{1,0,0}` | **Very high** as environment directional light; **conditional** as literal sun |
| **Secondary global environment direction** | `envCtx.dirLight2.params.dir` | Every frame | World space; exact opposite of light1 in time mode | Independently colored; not safe to discard as “just inverse sun” in fixed-light scenes | **Very high** |
| **Primary direct-light RGB** | post-adjustment `envCtx.dirLight1.params.dir.color[]` | Every frame | RGB 0–255, continuous/table-interpolated except setting boundaries/overrides | Includes environment adjustments such as rain; no separate intensity scalar | **Very high** |
| **Secondary direct-light RGB** | `envCtx.dirLight2.params.dir.color[]` | Every frame | Same | Can contribute independently | **Very high** |
| **Ambient/environment RGB** | `play->lightCtx.ambientColor[]` after `Environment_UpdateLights` | Every frame | RGB 0–255 | Includes `adjLightSettings`, weather, gameplay adjustments | **Very high** |
| **Simulation/celestial time** | `CURRENT_TIME` / `gSaveContext.save.time` | Usually advances every frame subject to time-stop rules | Cyclic continuous-ish 16-bit clock | Can freeze in menus/cutscenes/scenes | **Very high** |
| **Environment interpolation time** | `gSaveContext.skyboxTime` | Every frame but conditionally held/remapped | Cyclic clock | **Can differ from CURRENT_TIME at sunrise**; fixed-time scenes quantize several ranges | **Very high** |
| **Day/night state** | `gSaveContext.save.isNight` | Updated every frame | Boolean | Flips exactly at 06:00/18:00 but is not a direct-sun intensity flag | **High**, but low value for direct-light strength |
| **Environment mode** | `envCtx.lightMode` | Scene/config lifetime | Discrete | Determines whether directions are celestial/time-derived or table-authored | **Very high** |
| **Time-light configuration transition** | `lightConfig`, `changeLightNextConfig`, `changeLightEnabled`, `changeDuration`, `changeLightTimer` | Frames while crossing/config changes | Discrete IDs + continuous transition weight | Collision/light-zone transitions can overlap TOD transitions | **Very high** |
| **Fixed-light setting transition** | `lightSetting`, `prevLightSetting`, `lightBlend`, overrides | Scene/room/gameplay dependent | Discrete IDs + continuous blend | Blend rate can come from setting data or override | **Very high** |
| **Visual sun position/state** | `envCtx.sunPos`, `envCtx.sunDisabled`, visual sun globals such as `sSunPrimAlpha` | Every frame | World/celestial position plus visibility state | Cutscene smoothing/bug; rain independently fades sun | **High** |
| **Rain/weather state** | `gWeatherMode`, `envCtx.precipitation[]`, storm/lightning state | Dynamic | Discrete + continuous precipitation counts | Directional RGB already contains some weather attenuation | **Very high** |
| **Local point/directional lights** | `LightContext.listHead → LightNode.info → LightInfo` | Scene- or actor-owned | Point positions in world space; direction vectors for directional lights | Seven raster slots; point binding method varies by room/actor flags | **Very high** |
| **Positional-light realization capability** | `roomCtx.curRoom.enablePosLights` plus actor flags | Room/draw lifetime | Boolean rendering-mode semantic | Does not indicate whether lights exist, only how they are represented | **High** |
| **Room/environment metadata** | `curRoom.type`, `environmentType`, skybox/sun flags | Room lifetime | Discrete | Not equivalent to indoor/outdoor | **High for raw metadata; low for “outdoor” classification** |
| **Local authored light zone** | `SurfaceType_GetLightSettingIndex(...)` | Depends on current collision surface | Discrete 5-bit index | Can change lighting while sky remains visually visible | **Very high** |

### Direct-sun strength/confidence deserves its own semantic quantity

There is **no native canonical `sunStrength` field**.

The most defensible game-side ingredients are:

1. the primary environment RGB **after** `adjLightSettings`;
2. whether the primary direction is above the receiver/horizon;
3. the current time-light/environment mode;
4. weather/precipitation and sun-disable presentation state;
5. transition state between environment configurations.

The legacy `ActorShadow_DrawFeet` algorithm gives direct source support for jointly considering RGB magnitude and positive vertical direction. fileciteturn21file0

**Possible implication for the modern renderer:** a future semantic bridge could expose a continuous “direct-light confidence” derived from those authoritative ingredients. Such a scalar would be a renderer interpretation, not something MM explicitly stores, and should therefore remain distinct from raw game state.

A renderer should specifically **not** use any of the following alone as an original-game `sunExists` equivalent:

`dirLight1 != 0`,
`CURRENT_TIME >= 06:00`,
`isNight == false`,
`sunDisabled == false`,
or skybox presence.

Each is contradicted by another part of the source semantics.

## Sunrise instability assessment and open questions

### Original-game mechanisms plausibly relevant to the observed Clock Town symptom

Without diagnosing RT64, the MM source does contain several sunrise-specific mechanisms capable of exposing a renderer that matches sunlight from only part of the environment state.

**Most significant: `CURRENT_TIME` and `skyboxTime` temporarily diverge at 06:00.**

Direction and `sunPos` use `CURRENT_TIME`.

Time-light setting selection and RGB interpolation use `skyboxTime`.

`skyboxTime` can remain below 06:00 until `CURRENT_TIME >= 06:00 + 0x10`.

Therefore a renderer may see a post-horizon direction concurrently with pre-sunrise direct-light RGB/ambient state, followed by a discrete `skyboxTime` catch-up. fileciteturn72file0

**Second: the time-light segment itself changes at 06:00.**

The 04:00–06:00 setting pair and 06:00–08:00 pair are not universally index-continuous. Which behavior Clock Town sees depends on its active `lightConfig` and scene data. fileciteturn71file0

**Third: direct-light geometry crosses a meaningful threshold at sunrise.**

At 06:00, `light1Dir.y ≈ 0`. A valid horizontal vector exists, but the legacy foot-shadow system explicitly rejects/non-weights lights without positive Y and multiplies strength by vertical direction. Immediately afterward the primary light becomes progressively more suitable for a cast shadow. fileciteturn21file0

**Fourth: `isNight` flips discretely at 06:00 while the rest of the lighting is continuous or table-interpolated.**

That Boolean is therefore especially unsafe as a source of direct-sun activation. fileciteturn72file0

**Fifth: visual-sun visibility has separate state.**

Rain can fade the visual sun to zero without invalidating its direction. Its altitude-dependent appearance uses another continuous function. `sunDisabled` is also separate from environment-light mode. fileciteturn73file0 fileciteturn74file0

**Sixth: local environment-setting transitions can overlap sunrise.**

A collision-surface light-setting/config request starts a 20-frame interpolation between entire time-light configurations. If a player is crossing such an authored zone, the system simultaneously has a time-of-day interpolation and an environment-config interpolation. fileciteturn51file3 fileciteturn5file1 fileciteturn72file0

### Sunrise timeline

A useful conceptual reconstruction is therefore:

| Time | Celestial direction | Table-driven lighting | Day/night flag | Visual sun |
|---|---|---|---|---|
| **Before 04:00** | Valid night-side orbit | Night setting | Night | Sun may be geometrically below horizon |
| **04:00–<06:00** | Continuously approaches horizon | Dawn interpolation, e.g. `3→12` in config 0 | Night | Altitude-derived sun color remains clamped while below horizon |
| **06:00 exactly** | Primary Y crosses ~0 | `skyboxTime` may still be pre-06:00 | **Immediately day** | Celestial position is at horizon |
| **06:00 to 06:00+0x10** | Continues into positive Y | Can remain on final pre-06:00 table interval | Day | Position continues independently |
| **At/after 06:00+0x10** | Continuous | `skyboxTime` catches up into 06:00–08:00 segment; potentially different setting IDs | Day | Continues |
| **06:00–08:00** | Sun climbs continuously | Sunrise/morning RGB interpolation | Day | Altitude-based sun appearance strengthens |
| **08:00–16:00** | Continuous orbit | Stable daytime setting | Day | Normal daytime |
| **16:00–17:00** | Descending | Day → evening interpolation | Day | Continuous |
| **17:00–18:00** | Descending toward horizon | Sunset → night setting interpolation | Day | Continuous |
| **18:00** | Y crosses ~0 in opposite direction | Still inside 17:00–19:00 table interpolation | **Immediately night** | Celestial sun reaches horizon |
| **18:00–19:00** | Direction continues below horizon | Table transition continues toward night until 19:00 | Night | Visual sun altitude state is gone/below horizon |
| **After 19:00** | Valid below-horizon orbit still exists | Fixed night setting | Night | No meaningful direct solar presentation |

This reveals a fundamental MM design property:

> **Sunrise/sunset are not one synchronized switch.** Celestial geometry, environment RGB, day/night state, visual-sun appearance and light-setting identity have separate boundaries.

The asymmetry is also notable. The special `skyboxTime` hold is specifically around **06:00**; the code shown does not contain an analogous 18:00 hold. The evening lighting table already starts transitioning at 17:00 and continues until 19:00, while `isNight` flips at 18:00. fileciteturn72file0

### What is established versus still unresolved

**Established from zeldaret/mm source**

The primary time-mode environment direction and visual sun normally share the same analytic orbit.

The visual sun and actual environment light remain separate pieces of state.

Time-mode direct/ambient RGB is driven by `skyboxTime`; direction is driven by `CURRENT_TIME`.

`skyboxTime` contains a specific 06:00 hold/catch-up behavior.

Time-light setting pairs change at 04:00, 06:00, 08:00, 16:00, 17:00 and 19:00.

Some configurations change to nonmatching setting IDs at 06:00.

Rain attenuates actual ambient and directional RGB and separately hides the visual sun.

Two global directional lights are used; in time mode their directions oppose each other.

Fixed-light scenes may use arbitrary authored directional vectors unrelated to the solar orbit.

Collision surfaces can request lighting configurations/settings with a 20-frame transition.

Local point/directional lights are explicit authored game objects.

Generic blob shadows principally provide floor grounding; the foot-shadow path is genuinely light-direction-aware.

**Strong inference from source**

`dirLight1` is the best primary “sun-like” source in time-based environments but should be called a **primary environment directional light**, not universally “the sun.”

Post-adjusted directional RGB is a more authoritative direct-light-strength input than raw table RGB.

A physically meaningful direct-sun confidence should decrease toward the horizon even when the direction vector remains well defined.

The legacy foot-shadow strength calculation strongly supports using both directional RGB and positive elevation when deciding whether a strong directional shadow is semantically warranted.

An open roof or visible sky does not automatically imply ordinary outdoor solar lighting; authored light-setting zones can deliberately override that expectation.

**Possible implications for the modern renderer**

Treat global direction, global direct-light RGB, ambient RGB, environment transition state, time phase and visual-sun/weather state as independent semantic inputs.

Preserve both `CURRENT_TIME` and `skyboxTime` semantics when investigating sunrise rather than reconstructing all lighting from one normalized time value.

Do not activate RT sun merely because the sun vector becomes valid or because `isNight` flips false.

Consider legacy actor grounding as a separate visual responsibility from physically directional RT sun shadows.

Reuse authored `LightInfo` point lights for future RT local lights instead of inferring them from emissive materials.

### Open questions and limitations

The largest unresolved item for the **specific Clock Town observation** is the exact active `Z2_TOWN` `EnvLightSettings[]` data and which `lightConfig` is active at the reported location/state. The repository XML confirms the current scene asset but does not itself expose the relevant binary environment-light records in the source fragment investigated here. fileciteturn55file0

Consequently, the engine-level 06:00 discontinuity is proven, but this report does **not** claim that Clock Town's actual light-setting RGB values necessarily contain a large visual discontinuity between its pre- and post-06:00 indices.

The exact final draw-time conditions for the sun disc beyond the traced `sunDisabled`, `sSunPrimAlpha`, altitude-derived `sSunColor`, `sSunEnvAlpha`, and `sunPos` update were not fully traced here. The source is sufficient to prove that visible-sun presentation and environment lighting are separate, but not to characterize every draw cutoff with confidence.

Several decompiled environment globals (`D_801F4F30`, `D_801F4F31`, `D_801F4F33` and related state) remain weakly named. Their observed control flow is usable—for example the 20-frame return/change behavior in `Environment_ChangeLightSetting`—but assigning higher-level semantic names beyond demonstrated behavior would be speculative. fileciteturn5file1

The exact mapping of all `En_Elf` internal flag combinations to Tatl versus other fairy behaviors is likewise not sufficiently semantically named to assert that every visually glowing fairy state emits a scene light.

Finally, the renderer-facing term **“direct-sun strength” is necessarily a new semantic abstraction**. MM supplies the inputs—direction, two directional RGB values, ambient RGB, weather, sun visibility, time and transition state—but not one canonical scalar. The closest direct evidence for how the original engine decides whether directional shadowing is visually meaningful is the light-aware foot-shadow weighting by brightness and positive vertical direction. That distinction should be preserved when later RT64 implementation work begins.