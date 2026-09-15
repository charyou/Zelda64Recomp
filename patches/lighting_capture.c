#include "patches.h"
#include "z64.h"
#include "graphics.h"
#include "rt64_extended_gbi.h"

// Capture-only fixed storage. Nothing is traversed or published unless the host
// explicitly arms a bounded capture.
static u32 captureToken;
static u32 captureDetailed;
static u32 playEpoch = 1;
static PlayState* lastPlay;
static u32 lastGameplayFrame;
static RecompLightingBindingFrame bindingFrame;
static RecompEnvironmentFog publishedEnvironment;
static u32 publishedEnvironmentAvailable;

static void copy_environment_packet(RecompEnvironmentFog* destination, const RecompEnvironmentFog* source) {
    volatile u32* destinationWords = (volatile u32*)destination;
    const volatile u32* sourceWords = (const volatile u32*)source;
    u32 i;
    for (i = 0; i < sizeof(RecompEnvironmentFog) / sizeof(u32); i++) {
        destinationWords[i] = sourceWords[i];
    }
}

u32 recomp_lighting_capture_token(void) { return captureToken; }
RecompLightingBindingFrame* recomp_lighting_capture_binding_frame(void) { return &bindingFrame; }

static void clear_capture_producers(void) {
    captureToken = 0;
    captureDetailed = 0;
    bindingFrame.token = 0;
}

static u32 pack_rgb(u8* rgb) {
    return ((u32)rgb[0] << 16) | ((u32)rgb[1] << 8) | rgb[2];
}

static void copy_game_snapshot(PlayState* play, u32 phase, RecompLightingGameSnapshot* out) {
    Player* player = GET_PLAYER(play);
    u32 i;
    bzero(out, sizeof(*out));
    out->schemaVersion = RECOMP_LIGHTING_CAPTURE_SCHEMA;
    out->phase = phase;
    out->playEpoch = playEpoch;
    out->gameplayFrame = play->gameplayFrames;
    out->sceneId = play->sceneId;
    out->sceneLayer = gSaveContext.sceneLayer;
    out->savedEntrance = gSaveContext.save.entrance;
    out->curSpawn = play->curSpawn;
    out->nextEntrance = play->nextEntrance;
    out->transitionTrigger = play->transitionTrigger;
    out->transitionType = play->transitionType;
    out->transitionMode = play->transitionMode;
    out->currentRoom = play->roomCtx.curRoom.num;
    out->previousRoom = play->roomCtx.prevRoom.num;
    out->roomLoadStatus = play->roomCtx.status;
    out->currentRoomSegmentValid = play->roomCtx.curRoom.segment != NULL;
    out->previousRoomSegmentValid = play->roomCtx.prevRoom.segment != NULL;
    out->currentRoomEnablePosLights = play->roomCtx.curRoom.enablePosLights;
    out->currentRoomBehavior1 = play->roomCtx.curRoom.behaviorType1;
    out->currentRoomBehavior2 = play->roomCtx.curRoom.behaviorType2;
    out->gameMode = gSaveContext.gameMode;
    out->cutsceneState = play->csCtx.state;
    out->cutsceneFrame = play->csCtx.curFrame;
    out->cutsceneScriptIndex = play->csCtx.scriptIndex;
    out->savedCutsceneIndex = gSaveContext.save.cutsceneIndex;
    out->currentCutsceneId = CutsceneManager_GetCurrentCsId();
    out->playInCutscene = Play_InCsMode(play);
    out->dayRaw = gSaveContext.save.day;
    out->currentDay = CURRENT_DAY;
    out->currentTime = CURRENT_TIME;
    out->skyboxTime = gSaveContext.skyboxTime;
    out->sceneTimeSpeed = play->envCtx.sceneTimeSpeed;
    out->weatherMode = gWeatherMode;
    out->stormRequest = play->envCtx.stormRequest;
    out->stormState = play->envCtx.stormState;
    out->lightningState = play->envCtx.lightningState;
    for (i = 0; i < ARRAY_COUNT(out->precipitation); i++) out->precipitation[i] = play->envCtx.precipitation[i];
    out->lightMode = play->envCtx.lightMode;
    out->lightConfig = play->envCtx.lightConfig;
    out->changeLightNextConfig = play->envCtx.changeLightNextConfig;
    out->changeLightEnabled = play->envCtx.changeLightEnabled;
    out->changeLightTimer = play->envCtx.changeLightTimer;
    out->changeDuration = play->envCtx.changeDuration;
    out->lightBlendEnabled = play->envCtx.lightBlendEnabled;
    out->lightSetting = play->envCtx.lightSetting;
    out->previousLightSetting = play->envCtx.prevLightSetting;
    out->lightSettingOverride = play->envCtx.lightSettingOverride;
    out->lightBlendRateOverride = play->envCtx.lightBlendRateOverride;
    out->lightBlend = play->envCtx.lightBlend;
    out->lightBlendOverride = play->envCtx.lightBlendOverride;
    {
        const s16* adjustment = (const s16*)&play->envCtx.adjLightSettings;
        for (i = 0; i < ARRAY_COUNT(out->adjustment); i++) out->adjustment[i] = adjustment[i];
    }
    {
        CurrentEnvLightSettings* settings = &play->envCtx.lightSettings;
        out->resolvedLightSettings[0] = settings->ambientColor[0];
        out->resolvedLightSettings[1] = settings->ambientColor[1];
        out->resolvedLightSettings[2] = settings->ambientColor[2];
        out->resolvedLightSettings[3] = settings->light1Dir[0];
        out->resolvedLightSettings[4] = settings->light1Dir[1];
        out->resolvedLightSettings[5] = settings->light1Dir[2];
        out->resolvedLightSettings[6] = settings->light1Color[0];
        out->resolvedLightSettings[7] = settings->light1Color[1];
        out->resolvedLightSettings[8] = settings->light1Color[2];
        out->resolvedLightSettings[9] = settings->light2Dir[0];
        out->resolvedLightSettings[10] = settings->light2Dir[1];
        out->resolvedLightSettings[11] = settings->light2Dir[2];
        out->resolvedLightSettings[12] = settings->light2Color[0];
        out->resolvedLightSettings[13] = settings->light2Color[1];
        out->resolvedLightSettings[14] = settings->light2Color[2];
        out->resolvedLightSettings[15] = settings->fogNear;
    }
    out->ambientRGB = pack_rgb(play->lightCtx.ambientColor);
    out->fogRGB = pack_rgb(play->lightCtx.fogColor);
    out->fogNear = play->lightCtx.fogNear;
    out->zFar = play->lightCtx.zFar;
    out->primaryDirection[0] = play->envCtx.dirLight1.params.dir.x;
    out->primaryDirection[1] = play->envCtx.dirLight1.params.dir.y;
    out->primaryDirection[2] = play->envCtx.dirLight1.params.dir.z;
    out->primaryRGB = pack_rgb(play->envCtx.dirLight1.params.dir.color);
    out->secondaryDirection[0] = play->envCtx.dirLight2.params.dir.x;
    out->secondaryDirection[1] = play->envCtx.dirLight2.params.dir.y;
    out->secondaryDirection[2] = play->envCtx.dirLight2.params.dir.z;
    out->secondaryRGB = pack_rgb(play->envCtx.dirLight2.params.dir.color);
    out->sunPosition[0] = play->envCtx.sunPos.x;
    out->sunPosition[1] = play->envCtx.sunPos.y;
    out->sunPosition[2] = play->envCtx.sunPos.z;
    out->cameraEye[0] = play->view.eye.x;
    out->cameraEye[1] = play->view.eye.y;
    out->cameraEye[2] = play->view.eye.z;
    out->cameraAt[0] = play->view.at.x;
    out->cameraAt[1] = play->view.at.y;
    out->cameraAt[2] = play->view.at.z;
    out->cameraUp[0] = play->view.up.x;
    out->cameraUp[1] = play->view.up.y;
    out->cameraUp[2] = play->view.up.z;
    if (player != NULL) {
        out->playerPosition[0] = player->actor.world.pos.x;
        out->playerPosition[1] = player->actor.world.pos.y;
        out->playerPosition[2] = player->actor.world.pos.z;
        out->playerRotation[0] = player->actor.shape.rot.x;
        out->playerRotation[1] = player->actor.shape.rot.y;
        out->playerRotation[2] = player->actor.shape.rot.z;
    }
    out->publishedEnvironmentAvailable = publishedEnvironmentAvailable;
    if (publishedEnvironmentAvailable) copy_environment_packet(&out->publishedEnvironment, &publishedEnvironment);

    // The post phase intentionally compares only scalar environment/spatial values.
    // It does not rescan the list solely for diagnostics.
    if (phase == 0) {
        LightNode* visited[RECOMP_LIGHTING_CAPTURE_MAX_NODES];
        LightNode* node = play->lightCtx.listHead;
        while (node != NULL) {
            RecompLightingNodeSnapshot* target;
            for (i = 0; i < out->nodeCount; i++) {
                if (visited[i] == node) {
                    out->nodeCycleDetected = true;
                    node = NULL;
                    break;
                }
            }
            if (node == NULL) break;
            if (out->nodeCount >= ARRAY_COUNT(out->nodes)) {
                out->nodeDropped++;
                break;
            }
            visited[out->nodeCount] = node;
            target = &out->nodes[out->nodeCount];
            target->ordinal = out->nodeCount++;
            if (node->info == NULL) {
                out->nodeMalformed = true;
                break;
            }
            target->type = node->info->type;
            target->environment1 = node->info == &play->envCtx.dirLight1;
            target->environment2 = node->info == &play->envCtx.dirLight2;
            if (node->info->type == LIGHT_DIRECTIONAL) {
                target->x = node->info->params.dir.x;
                target->y = node->info->params.dir.y;
                target->z = node->info->params.dir.z;
                target->rgb = pack_rgb(node->info->params.dir.color);
            } else if (node->info->type == LIGHT_POINT_GLOW || node->info->type == LIGHT_POINT_NOGLOW) {
                target->x = node->info->params.point.x;
                target->y = node->info->params.point.y;
                target->z = node->info->params.point.z;
                target->radius = node->info->params.point.radius;
                target->rgb = pack_rgb(node->info->params.point.color);
                target->glow = node->info->params.point.drawGlow;
            }
            node = node->next;
        }
    }
}

void recomp_lighting_capture_begin_play(PlayState* play, RecompEnvironmentFog* environment) {
    RecompLightingGameSnapshot snapshot;
    GraphicsContext* gfxCtx;
    u32 captureState = recomp_lighting_capture_is_armed();
    if (!captureState) {
        clear_capture_producers();
        publishedEnvironmentAvailable = false;
        return;
    }
    captureDetailed = captureState == 1;
    copy_environment_packet(&publishedEnvironment, environment);
    publishedEnvironmentAvailable = true;
    // The runtime may reconstruct a PlayState at the same address. A reset of
    // PlayState.gameplayFrames is therefore part of instance identity; pointer
    // equality alone is not a sufficient lifetime test.
    if ((lastPlay != play) || (play->gameplayFrames < lastGameplayFrame)) {
        lastPlay = play;
        playEpoch++;
    }
    lastGameplayFrame = play->gameplayFrames;
    copy_game_snapshot(play, 0, &snapshot);
    captureToken = recomp_lighting_capture_begin(&snapshot);
    bzero(&bindingFrame, sizeof(bindingFrame));
    bindingFrame.schemaVersion = RECOMP_LIGHTING_CAPTURE_SCHEMA;
    bindingFrame.token = captureDetailed ? captureToken : 0;
    bindingFrame.receiptCapacity = 512;
    if (captureToken == 0) return;
    gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    gEXLightingCapture(POLY_OPA_DISP++, captureToken, 0);
    gEXLightingCapture(POLY_XLU_DISP++, captureToken, 1);
    bindingFrame.opaMarkerEmitted = true;
    bindingFrame.xluMarkerEmitted = true;
    CLOSE_DISPS(gfxCtx);
}

void recomp_lighting_capture_end_play(PlayState* play) {
    RecompLightingGameSnapshot snapshot;
    if (captureToken == 0) return;
    copy_game_snapshot(play, 1, &snapshot);
    recomp_lighting_capture_post(captureToken, &snapshot);
}

void recomp_lighting_capture_finalize_task(void) {
    if ((captureToken != 0) && captureDetailed) recomp_lighting_capture_finalize(&bindingFrame);
    clear_capture_producers();
}
