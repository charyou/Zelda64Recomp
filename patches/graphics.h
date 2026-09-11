#ifndef __PATCH_GRAPHICS_H__
#define __PATCH_GRAPHICS_H__

#include "patch_helpers.h"
#include "../include/z64recomp_atmosphere_api.h"

DECLARE_FUNC(void, recomp_get_window_resolution, u32*, u32*);
DECLARE_FUNC(float, recomp_get_target_aspect_ratio, float);
DECLARE_FUNC(s32, recomp_get_target_framerate, s32);
DECLARE_FUNC(s32, recomp_high_precision_fb_enabled);
DECLARE_FUNC(float, recomp_get_resolution_scale);

typedef struct RecompEnvironmentFog {
    u32 valid;
    u32 rgb;
    s32 fogNear;
    s32 zFar;
    float sunX;
    float sunY;
    float sunZ;
    float cameraX;
    float cameraY;
    float cameraZ;
    float viewX;
    float viewY;
    float viewZ;
    float referenceHeight;
    u32 outdoor;
    u32 rain;
    u32 snow;
    u32 storm;
    u32 expandedOutdoor;
    u32 atmosphereOverrideMask;
    float baseHeightBlend;
    float morningHeightBlend;
    float scaleHeightFraction;
    float densityVariation;
    float directionalScattering;
    float saturatedFogHeightBudget;
    float clearAirFarTransmittance;
    float wetAirFarTransmittance;
    float waterInfluence;
    u32 ambientRGB;
    float skyFillWeight;
} RecompEnvironmentFog;

DECLARE_FUNC(void, recomp_set_environment_fog, RecompEnvironmentFog* fog);

#endif
