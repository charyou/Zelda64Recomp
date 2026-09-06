#ifndef __Z64RECOMP_ATMOSPHERE_API_H__
#define __Z64RECOMP_ATMOSPHERE_API_H__

#if defined(MIPS)
#include "PR/ultratypes.h"
#define Z64RECOMP_ATMOSPHERE_U32 u32
#else
#include <stdint.h>
#define Z64RECOMP_ATMOSPHERE_U32 uint32_t
#endif

typedef enum RecompAtmosphereOverrideField {
    RECOMP_ATMOSPHERE_OVERRIDE_BASE_HEIGHT_BLEND = (1 << 0),
    RECOMP_ATMOSPHERE_OVERRIDE_MORNING_HEIGHT_BLEND = (1 << 1),
    RECOMP_ATMOSPHERE_OVERRIDE_SCALE_HEIGHT_FRACTION = (1 << 2),
    RECOMP_ATMOSPHERE_OVERRIDE_DENSITY_VARIATION = (1 << 3),
    RECOMP_ATMOSPHERE_OVERRIDE_DIRECTIONAL_SCATTERING = (1 << 4),
    RECOMP_ATMOSPHERE_OVERRIDE_SATURATED_FOG_HEIGHT_BUDGET = (1 << 5),
    RECOMP_ATMOSPHERE_OVERRIDE_CLEAR_AIR_TRANSMITTANCE = (1 << 6),
    RECOMP_ATMOSPHERE_OVERRIDE_WET_AIR_TRANSMITTANCE = (1 << 7),
    RECOMP_ATMOSPHERE_OVERRIDE_OUTDOOR = (1 << 8),
    RECOMP_ATMOSPHERE_OVERRIDE_WATER_INFLUENCE = (1 << 9),
} RecompAtmosphereOverrideField;

typedef enum RecompAtmosphereOutdoorOverride {
    RECOMP_ATMOSPHERE_OUTDOOR_AUTO = 0,
    RECOMP_ATMOSPHERE_OUTDOOR_FORCE_OFF = 1,
    RECOMP_ATMOSPHERE_OUTDOOR_FORCE_ON = 2,
} RecompAtmosphereOutdoorOverride;

// Per-frame art direction supplied by code mods through recomp_on_atmosphere_override.
// Set only the fields the mod owns in overrideMask. Unset fields use renderer defaults.
typedef struct RecompAtmosphereOverride {
    Z64RECOMP_ATMOSPHERE_U32 overrideMask;
    float baseHeightBlend;
    float morningHeightBlend;
    float scaleHeightFraction;
    float densityVariation;
    float directionalScattering;
    float saturatedFogHeightBudget;
    float clearAirFarTransmittance;
    float wetAirFarTransmittance;
    Z64RECOMP_ATMOSPHERE_U32 outdoorOverride;
    // Appended extension: nearby active water coverage, [0, 1]. Set the mask to
    // replace this automatic signal (including zero for unsupported/custom water).
    float waterInfluence;
} RecompAtmosphereOverride;

#undef Z64RECOMP_ATMOSPHERE_U32

#endif
