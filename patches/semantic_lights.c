#include "patches.h"
#include "z64.h"
#include "graphics.h"
#include "rt64_extended_gbi.h"

void Lights_BindPoint(Lights*, LightParams*, PlayState*);
void Lights_BindPointWithReference(Lights*, LightParams*, Vec3f*);
void Lights_BindDirectional(Lights*, LightParams*, void*);

typedef struct SemanticSource {
    float positionRange[4];
    float colorStrength[4];
    float response[4];
} SemanticSource;

// One replay's binding receipts. Overflow loses enhancement, never original lights.
typedef struct LightReceipt {
    Lights* owner;
    Lights snapshot;
    SemanticSource sources[7];
    u32 bindOrdinal;
} LightReceipt;
static LightReceipt receipts[512];
static s32 receiptCount;

void recomp_reset_light_receipts(void) { receiptCount = 0; }

static RecompLightingBindingFrame* capture_frame(void) {
    extern RecompLightingBindingFrame* recomp_lighting_capture_binding_frame(void);
    RecompLightingBindingFrame* frame = recomp_lighting_capture_binding_frame();
    return frame->token != 0 ? frame : NULL;
}

static RecompLightingBindAttempt* capture_attempt(RecompLightingBindingFrame* frame) {
    if (frame == NULL) return NULL;
    if (frame->attemptCount >= RECOMP_LIGHTING_CAPTURE_MAX_ATTEMPTS) {
        frame->attemptDropped++;
        return NULL;
    }
    return &frame->attempts[frame->attemptCount++];
}

static s32 same_bytes(const void* a, const void* b, s32 size) {
    const u8* x = a;
    const u8* y = b;
    s32 i;
    for (i = 0; i < size; i++) if (x[i] != y[i]) return false;
    return true;
}

// Validate the actual realization, including modifications by replacement binders.
// No identity, approximate RGB/direction matching, or actor/scene gates.
static s32 owns_point(Light* light, LightPoint* p, Vec3f* reference, s32 positional) {
    s32 i;
    float weight = 1;
    if (p->radius <= 0) return false;
    if (positional) {
        s32 kq = CLAMP(4500000.0f / ((float)p->radius * p->radius), 20, 255);
        if (light->p.pos[0] != p->x || light->p.pos[1] != p->y || light->p.pos[2] != p->z ||
            light->p.unk3 != 8 || light->p.unk7 != (u8)-1 || light->p.unkE != kq) return false;
    } else {
        float delta[3];
        float distance;
        float scale;
        if (reference == NULL) return false;
        delta[0] = p->x - reference->x;
        delta[1] = p->y - reference->y;
        delta[2] = p->z - reference->z;
        distance = sqrtf(SQ(delta[0]) + SQ(delta[1]) + SQ(delta[2]));
        weight = 1 - SQ(distance / p->radius);
        if (weight <= 0) return false;
        scale = distance < 1 ? 120 : 120 / distance;
        for (i = 0; i < 3; i++) if (light->l.dir[i] != (s8)(delta[i] * scale)) return false;
    }
    for (i = 0; i < 3; i++) {
        u8 color = p->color[i] * weight;
        if (light->l.col[i] != color || light->l.colc[i] != color) return false;
    }
    return true;
}

RECOMP_PATCH void Lights_BindAll(Lights* lights, LightNode* node, Vec3f* refPos, PlayState* play) {
    LightReceipt* receipt = NULL;
    RecompLightingBindingFrame* frame = capture_frame();
    s32 positional = refPos == NULL && lights->enablePosLights == 1;
    u32 bindOrdinal = frame != NULL ? ++frame->bindCount : 0;
    u32 attemptOrdinal = 0;
    u32 initialSlots = lights->numLights;
    s32 i;
    // Invalidate any earlier receipt for this group, including address reuse.
    for (i = 0; i < receiptCount; i++) if (receipts[i].owner == lights) {
        receipts[i].owner = NULL;
        if (frame != NULL) frame->receiptInvalidated++;
    }
    if (receiptCount < ARRAY_COUNT(receipts)) {
        receipt = &receipts[receiptCount++];
        bzero(receipt, sizeof(*receipt));
        receipt->owner = lights;
        receipt->bindOrdinal = bindOrdinal;
        if (frame != NULL) frame->receiptCount++;
    } else if (frame != NULL) {
        frame->receiptOverflow++;
    }
    for (; node != NULL; node = node->next) {
        LightInfo* info = node->info;
        s32 slot = lights->numLights;
        RecompLightingBindAttempt* attempt = capture_attempt(frame);
        if (attempt != NULL) {
            bzero(attempt, sizeof(*attempt));
            attempt->bindOrdinal = bindOrdinal;
            attempt->attemptOrdinal = attemptOrdinal++;
            attempt->type = info->type;
            attempt->positionalMode = positional;
            attempt->refPresent = refPos != NULL;
            if (refPos != NULL) {
                attempt->refPosition[0] = refPos->x;
                attempt->refPosition[1] = refPos->y;
                attempt->refPosition[2] = refPos->z;
            }
            attempt->initialSlots = initialSlots;
            attempt->beforeSlots = lights->numLights;
            attempt->preexistingFullSlots = lights->numLights >= 7;
        }
        if (info->type == LIGHT_DIRECTIONAL) {
            if (attempt != NULL) {
                attempt->parameterKind = 2; // directional
                attempt->parametersObserved = true;
                attempt->direction[0] = info->params.dir.x;
                attempt->direction[1] = info->params.dir.y;
                attempt->direction[2] = info->params.dir.z;
                attempt->directionRGB = ((u32)info->params.dir.color[0] << 16) |
                    ((u32)info->params.dir.color[1] << 8) | info->params.dir.color[2];
            }
            Lights_BindDirectional(lights, &info->params, NULL);
        }
        else if (info->type == LIGHT_POINT_GLOW || info->type == LIGHT_POINT_NOGLOW) {
            LightPoint* p = &info->params.point;
            if (attempt != NULL) {
                attempt->parameterKind = 1; // point
                attempt->parametersObserved = true;
                attempt->x = p->x;
                attempt->y = p->y;
                attempt->z = p->z;
                attempt->radius = p->radius;
                attempt->rgb = ((u32)p->color[0] << 16) | ((u32)p->color[1] << 8) | p->color[2];
            }
            if (positional) Lights_BindPoint(lights, &info->params, play);
            else Lights_BindPointWithReference(lights, &info->params, refPos);
            if (receipt != NULL && slot < 7 && lights->numLights == slot + 1) {
                s32 verified = owns_point(&lights->l.l[slot], &info->params.point, refPos, positional);
                if (attempt != NULL) attempt->ownsPoint = verified;
                if (verified) {
                SemanticSource* source = &receipt->sources[slot];
                *source = (SemanticSource){
                    { p->x, p->y, p->z, p->radius },
                    { p->color[0] / 255.0f, p->color[1] / 255.0f, p->color[2] / 255.0f, 1.0f },
                    // Small authored sources have continuously weaker, shorter shadows.
                    { CLAMP(p->radius / 160.0f, 0.0f, 1.0f), p->radius, 0.0f, 0.35f }
                };
                }
            }
        } else if (attempt != NULL) {
            attempt->outcome = 4; // unsupported type
        }
        if (attempt != NULL) {
            attempt->afterSlots = lights->numLights;
            attempt->returnedSlot = lights->numLights == slot + 1 ? slot : 0xFFFFFFFF;
            if (attempt->outcome != 4) {
                if (lights->numLights != slot + 1) attempt->outcome = 3; // not bound; reason unobserved
                else if (attempt->ownsPoint) attempt->outcome = 1; // bound verified
                else attempt->outcome = 2; // bound unverified
            }
        }
    }
    if (receipt != NULL) {
        volatile u8* dst = (volatile u8*)&receipt->snapshot;
        const u8* src = (const u8*)lights;
        for (i = 0; i < sizeof(Lights); i++) dst[i] = src[i];
    }
}

RECOMP_PATCH void Lights_Draw(Lights* lights, GraphicsContext* gfxCtx) {
    LightReceipt* receipt = NULL;
    LightReceipt* observedReceipt = NULL;
    RecompLightingBindingFrame* frame = capture_frame();
    RecompLightingDrawReceiptEvent* drawEvent = NULL;
    u32 annotatedSourceSlots = 0;
    s32 i;
    if (frame != NULL) {
        u32 drawOrdinal = ++frame->drawCount;
        if (frame->drawEventCount < RECOMP_LIGHTING_CAPTURE_MAX_DRAW_EVENTS) {
            drawEvent = &frame->drawEvents[frame->drawEventCount++];
            bzero(drawEvent, sizeof(*drawEvent));
            drawEvent->drawOrdinal = drawOrdinal;
            drawEvent->bindOrdinal = 0xFFFFFFFF;
            drawEvent->numLights = lights->numLights;
        } else {
            frame->drawEventDropped++;
        }
    }
    for (i = receiptCount - 1; i >= 0; i--) {
        if (receipts[i].owner == lights) {
            observedReceipt = &receipts[i];
            if (frame != NULL) frame->receiptFound++;
            if (same_bytes(lights, &receipts[i].snapshot, sizeof(Lights))) {
                receipt = &receipts[i];
                if (frame != NULL) frame->receiptEqual++;
            } else if (frame != NULL) {
                frame->receiptReusedOrMismatched++;
            }
            receipts[i].owner = NULL; // A later uninstrumented reuse cannot inherit authority.
            if (frame != NULL) frame->receiptConsumed++;
            break;
        }
    }
    if (drawEvent != NULL) {
        drawEvent->receiptFound = observedReceipt != NULL;
        drawEvent->receiptEqual = receipt != NULL;
        drawEvent->receiptConsumed = observedReceipt != NULL;
        drawEvent->receiptReusedOrMismatched = observedReceipt != NULL && receipt == NULL;
        if (observedReceipt != NULL) drawEvent->bindOrdinal = observedReceipt->bindOrdinal;
    }
    OPEN_DISPS(gfxCtx);
    gSPNumLights(POLY_OPA_DISP++, lights->numLights);
    gSPNumLights(POLY_XLU_DISP++, lights->numLights);
    for (i = 0; i < lights->numLights; i++) {
        gSPLight(POLY_OPA_DISP++, &lights->l.l[i], i + 1);
        gSPLight(POLY_XLU_DISP++, &lights->l.l[i], i + 1);
        if (receipt != NULL && i < 7 && receipt->sources[i].positionRange[3] > 0) {
            SemanticSource* source = GRAPH_ALLOC(gfxCtx, sizeof(SemanticSource));
            Gfx* opaCommand;
            Gfx* xluCommand;
            *source = receipt->sources[i];
            annotatedSourceSlots++;
            opaCommand = POLY_OPA_DISP;
            gEXSetLightSource(POLY_OPA_DISP++, i, source);
            xluCommand = POLY_XLU_DISP;
            gEXSetLightSource(POLY_XLU_DISP++, i, source);
            if (frame != NULL) {
                Gfx* commands[2] = { opaCommand, xluCommand };
                s32 stream;
                for (stream = 0; stream < 2; stream++) {
                    if (frame->emissionCount < RECOMP_LIGHTING_CAPTURE_MAX_EMISSIONS) {
                        RecompLightingAnnotationEmission* emission = &frame->emissions[frame->emissionCount++];
                        const u32* payload = (const u32*)source;
                        s32 word;
                        emission->token = frame->token;
                        emission->stream = stream;
                        emission->slot = i;
                        emission->bindOrdinal = receipt->bindOrdinal;
                        emission->rdramAddress = OS_K0_TO_PHYSICAL(commands[stream]);
                        emission->commandWords[0] = commands[stream][0].words.w0;
                        emission->commandWords[1] = commands[stream][0].words.w1;
                        emission->commandWords[2] = commands[stream][1].words.w0;
                        emission->commandWords[3] = commands[stream][1].words.w1;
                        for (word = 0; word < 12; word++) emission->payloadWords[word] = payload[word];
                    } else {
                        frame->emissionDropped++;
                    }
                }
            }
        }
    }
    gSPLight(POLY_OPA_DISP++, &lights->l.a, i + 1);
    gSPLight(POLY_XLU_DISP++, &lights->l.a, i + 1);
    CLOSE_DISPS(gfxCtx);
    if (drawEvent != NULL) drawEvent->annotatedSourceSlots = annotatedSourceSlots;
}
