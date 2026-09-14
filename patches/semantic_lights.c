#include "patches.h"
#include "z64.h"
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
} LightReceipt;
static LightReceipt receipts[512];
static s32 receiptCount;

void recomp_reset_light_receipts(void) { receiptCount = 0; }

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
    s32 positional = refPos == NULL && lights->enablePosLights == 1;
    s32 i;
    // Invalidate any earlier receipt for this group, including address reuse.
    for (i = 0; i < receiptCount; i++) if (receipts[i].owner == lights) receipts[i].owner = NULL;
    if (receiptCount < ARRAY_COUNT(receipts)) {
        receipt = &receipts[receiptCount++];
        bzero(receipt, sizeof(*receipt));
        receipt->owner = lights;
    }
    for (; node != NULL; node = node->next) {
        LightInfo* info = node->info;
        s32 slot = lights->numLights;
        if (info->type == LIGHT_DIRECTIONAL) Lights_BindDirectional(lights, &info->params, NULL);
        else if (info->type == LIGHT_POINT_GLOW || info->type == LIGHT_POINT_NOGLOW) {
            if (positional) Lights_BindPoint(lights, &info->params, play);
            else Lights_BindPointWithReference(lights, &info->params, refPos);
            if (receipt != NULL && slot < 7 && lights->numLights == slot + 1 &&
                owns_point(&lights->l.l[slot], &info->params.point, refPos, positional)) {
                LightPoint* p = &info->params.point;
                SemanticSource* source = &receipt->sources[slot];
                *source = (SemanticSource){
                    { p->x, p->y, p->z, p->radius },
                    { p->color[0] / 255.0f, p->color[1] / 255.0f, p->color[2] / 255.0f, 1.0f },
                    // Small authored sources have continuously weaker, shorter shadows.
                    { CLAMP(p->radius / 160.0f, 0.0f, 1.0f), p->radius, 0.0f, 0.0f }
                };
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
    s32 i;
    for (i = receiptCount - 1; i >= 0; i--) {
        if (receipts[i].owner == lights) {
            if (same_bytes(lights, &receipts[i].snapshot, sizeof(Lights))) receipt = &receipts[i];
            receipts[i].owner = NULL; // A later uninstrumented reuse cannot inherit authority.
            break;
        }
    }
    OPEN_DISPS(gfxCtx);
    gSPNumLights(POLY_OPA_DISP++, lights->numLights);
    gSPNumLights(POLY_XLU_DISP++, lights->numLights);
    for (i = 0; i < lights->numLights; i++) {
        gSPLight(POLY_OPA_DISP++, &lights->l.l[i], i + 1);
        gSPLight(POLY_XLU_DISP++, &lights->l.l[i], i + 1);
        if (receipt != NULL && i < 7 && receipt->sources[i].positionRange[3] > 0) {
            SemanticSource* source = GRAPH_ALLOC(gfxCtx, sizeof(SemanticSource));
            *source = receipt->sources[i];
            gEXSetLightSource(POLY_OPA_DISP++, i, source);
            gEXSetLightSource(POLY_XLU_DISP++, i, source);
        }
    }
    gSPLight(POLY_OPA_DISP++, &lights->l.a, i + 1);
    gSPLight(POLY_XLU_DISP++, &lights->l.a, i + 1);
    CLOSE_DISPS(gfxCtx);
}
