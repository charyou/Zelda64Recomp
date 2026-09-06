# Atmospheric Fog Mod Overrides

Atmospheric fog exposes the `recomp_on_atmosphere_override` base-game event for code mods. The event runs once per gameplay frame after Majora's Mask has resolved the current environment and before the display list is submitted. Overrides affect only RT64's enhanced Atmospheric replay; Original/Native rendering and MM's `LightContext` remain unchanged.

Use the public definitions in `include/z64recomp_atmosphere_api.h` in a mod project and register a base-game callback:

```c
#include "global.h"
#include "modding.h"
#include "z64recomp_atmosphere_api.h"

RECOMP_CALLBACK("*", recomp_on_atmosphere_override)
void my_atmosphere_override(PlayState* play, RecompAtmosphereOverride* atmosphere) {
    if ((play->sceneId == SCENE_20SICHITAI) && (play->roomCtx.curRoom.num == 0)) {
        atmosphere->overrideMask |=
            RECOMP_ATMOSPHERE_OVERRIDE_BASE_HEIGHT_BLEND |
            RECOMP_ATMOSPHERE_OVERRIDE_DENSITY_VARIATION;
        atmosphere->baseHeightBlend = 0.30f;
        atmosphere->densityVariation = 0.18f;
    }
}
```

The callback receives initialized renderer defaults, so a mod only needs to set the mask bits and values it owns. The object is reset every frame; scene-, room-, cutscene-, time-, or weather-specific logic therefore belongs directly in the callback. When several mods claim the same bit, normal callback ordering determines the final value.

Available fields are:

| Mask bit | Value | Default | Meaning |
|---|---:|---:|---|
| `RECOMP_ATMOSPHERE_OVERRIDE_BASE_HEIGHT_BLEND` | `baseHeightBlend` | 0.22 | Base share of authored fog redistributed into the height medium. |
| `RECOMP_ATMOSPHERE_OVERRIDE_MORNING_HEIGHT_BLEND` | `morningHeightBlend` | 0.69 | Height-medium share near the morning horizon. |
| `RECOMP_ATMOSPHERE_OVERRIDE_SCALE_HEIGHT_FRACTION` | `scaleHeightFraction` | 0.015 | Height scale as a fraction of MM's resolved far distance. |
| `RECOMP_ATMOSPHERE_OVERRIDE_DENSITY_VARIATION` | `densityVariation` | 0.12 | Amplitude of slow world-space density variation. |
| `RECOMP_ATMOSPHERE_OVERRIDE_DIRECTIONAL_SCATTERING` | `directionalScattering` | 0.25 | Sun-direction scattering strength. |
| `RECOMP_ATMOSPHERE_OVERRIDE_SATURATED_FOG_HEIGHT_BUDGET` | `saturatedFogHeightBudget` | 0.03 | Remaining redistribution budget when MM fog is saturated. |
| `RECOMP_ATMOSPHERE_OVERRIDE_CLEAR_AIR_TRANSMITTANCE` | `clearAirFarTransmittance` | 0.90 | Outdoor clear-air transmittance at the resolved far distance; lower means more haze. |
| `RECOMP_ATMOSPHERE_OVERRIDE_WET_AIR_TRANSMITTANCE` | `wetAirFarTransmittance` | 0.65 | Fully wet/stormy-air transmittance at the resolved far distance. |
| `RECOMP_ATMOSPHERE_OVERRIDE_OUTDOOR` | `outdoorOverride` | `AUTO` | Force the current frame's outdoor classification on or off. |

For the outdoor override, set the mask bit and use `RECOMP_ATMOSPHERE_OUTDOOR_FORCE_ON` or `RECOMP_ATMOSPHERE_OUTDOOR_FORCE_OFF`. Leaving the bit unset, or selecting `RECOMP_ATMOSPHERE_OUTDOOR_AUTO`, preserves the renderer's automatic classification. This is the intended way to opt a shop, unusual cutscene view, or area without a normal skybox into or out of clear-/wet-air haze.

F1's `Atmosphere` tab shows the current override mask and effective workload values. Masked values take precedence over session-local F1 sliders; unmasked values remain live-tunable.
