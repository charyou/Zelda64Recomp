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
    float primaryDirection[3];
    u32 primaryRGB;
    float secondaryDirection[3];
    u32 secondaryRGB;
    float localBounceStrength;
} RecompEnvironmentFog;

DECLARE_FUNC(void, recomp_set_environment_fog, RecompEnvironmentFog* fog);

#define RECOMP_LIGHTING_CAPTURE_SCHEMA 2
#define RECOMP_LIGHTING_CAPTURE_MAX_NODES 64
#define RECOMP_LIGHTING_CAPTURE_MAX_ATTEMPTS 2048
#define RECOMP_LIGHTING_CAPTURE_MAX_EMISSIONS 1024
#define RECOMP_LIGHTING_CAPTURE_MAX_DRAW_EVENTS 1024

typedef struct RecompLightingNodeSnapshot {
    u32 ordinal;
    u32 type;
    s32 x;
    s32 y;
    s32 z;
    s32 radius;
    u32 rgb;
    u32 glow;
    u32 environment1;
    u32 environment2;
} RecompLightingNodeSnapshot;

typedef struct RecompLightingGameSnapshot {
    u32 schemaVersion;
    u32 phase;
    u32 playEpoch;
    u32 gameplayFrame;
    s32 sceneId;
    s32 sceneLayer;
    s32 savedEntrance;
    u32 curSpawn;
    u32 nextEntrance;
    s32 transitionTrigger;
    u32 transitionType;
    u32 transitionMode;
    s32 currentRoom;
    s32 previousRoom;
    s32 roomLoadStatus;
    u32 currentRoomSegmentValid;
    u32 previousRoomSegmentValid;
    u32 currentRoomEnablePosLights;
    u32 currentRoomBehavior1;
    u32 currentRoomBehavior2;
    s32 gameMode;
    u32 cutsceneState;
    u32 cutsceneFrame;
    u32 cutsceneScriptIndex;
    s32 savedCutsceneIndex;
    s32 currentCutsceneId;
    u32 playInCutscene;
    u32 dayRaw;
    u32 currentDay;
    u32 currentTime;
    u32 skyboxTime;
    s32 sceneTimeSpeed;
    u32 weatherMode;
    u32 stormRequest;
    u32 stormState;
    u32 lightningState;
    u32 precipitation[5];
    u32 lightMode;
    u32 lightConfig;
    u32 changeLightNextConfig;
    u32 changeLightEnabled;
    u32 changeLightTimer;
    u32 changeDuration;
    u32 lightBlendEnabled;
    u32 lightSetting;
    u32 previousLightSetting;
    u32 lightSettingOverride;
    u32 lightBlendRateOverride;
    float lightBlend;
    u32 lightBlendOverride;
    s32 adjustment[14];
    s32 resolvedLightSettings[16];
    u32 ambientRGB;
    u32 fogRGB;
    s32 fogNear;
    s32 zFar;
    s32 primaryDirection[3];
    u32 primaryRGB;
    s32 secondaryDirection[3];
    u32 secondaryRGB;
    float sunPosition[3];
    float cameraEye[3];
    float cameraAt[3];
    float cameraUp[3];
    float playerPosition[3];
    s32 playerRotation[3];
    u32 publishedEnvironmentAvailable;
    RecompEnvironmentFog publishedEnvironment;
    u32 nodeCount;
    u32 nodeDropped;
    u32 nodeCycleDetected;
    u32 nodeMalformed;
    RecompLightingNodeSnapshot nodes[RECOMP_LIGHTING_CAPTURE_MAX_NODES];
} RecompLightingGameSnapshot;

typedef struct RecompLightingBindAttempt {
    u32 bindOrdinal;
    u32 attemptOrdinal;
    u32 type;
    u32 positionalMode;
    u32 refPresent;
    float refPosition[3];
    u32 parameterKind;
    u32 parametersObserved;
    s32 x;
    s32 y;
    s32 z;
    s32 radius;
    u32 rgb;
    s32 direction[3];
    u32 directionRGB;
    u32 initialSlots;
    u32 beforeSlots;
    u32 afterSlots;
    u32 preexistingFullSlots;
    u32 returnedSlot;
    u32 ownsPoint;
    u32 outcome;
} RecompLightingBindAttempt;

typedef struct RecompLightingAnnotationEmission {
    u32 token;
    u32 stream;
    u32 slot;
    u32 bindOrdinal;
    u32 rdramAddress;
    u32 commandWords[4];
    u32 payloadWords[12];
} RecompLightingAnnotationEmission;

typedef struct RecompLightingDrawReceiptEvent {
    u32 drawOrdinal;
    u32 bindOrdinal;
    u32 numLights;
    u32 receiptFound;
    u32 receiptEqual;
    u32 receiptConsumed;
    u32 receiptReusedOrMismatched;
    u32 annotatedSourceSlots;
} RecompLightingDrawReceiptEvent;

typedef struct RecompLightingBindingFrame {
    u32 schemaVersion;
    u32 token;
    u32 bindCount;
    u32 bindDropped;
    u32 attemptCount;
    u32 attemptDropped;
    u32 receiptCapacity;
    u32 receiptCount;
    u32 receiptOverflow;
    u32 receiptInvalidated;
    u32 receiptFound;
    u32 receiptEqual;
    u32 receiptConsumed;
    u32 receiptReusedOrMismatched;
    u32 drawCount;
    u32 drawEventCount;
    u32 drawEventDropped;
    u32 emissionCount;
    u32 emissionDropped;
    u32 opaMarkerEmitted;
    u32 xluMarkerEmitted;
    RecompLightingBindAttempt attempts[RECOMP_LIGHTING_CAPTURE_MAX_ATTEMPTS];
    RecompLightingDrawReceiptEvent drawEvents[RECOMP_LIGHTING_CAPTURE_MAX_DRAW_EVENTS];
    RecompLightingAnnotationEmission emissions[RECOMP_LIGHTING_CAPTURE_MAX_EMISSIONS];
} RecompLightingBindingFrame;

DECLARE_FUNC(u32, recomp_lighting_capture_begin, RecompLightingGameSnapshot* snapshot);
DECLARE_FUNC(void, recomp_lighting_capture_post, u32 token, RecompLightingGameSnapshot* snapshot);
DECLARE_FUNC(void, recomp_lighting_capture_finalize, RecompLightingBindingFrame* bindings);
DECLARE_FUNC(u32, recomp_lighting_capture_is_armed);
// Nonzero when behind-camera opaque room entries are submitted as renderer occluders.
DECLARE_FUNC(u32, recomp_room_occluder_completion_enabled);

#endif
