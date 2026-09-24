#include <cmath>
#include <cstdlib>
#include <cstring>

#include "recomp.h"
#include "librecomp/overlays.hpp"
#include "zelda_config.h"
#include "recomp_input.h"
#include "recomp_ui.h"
#include "zelda_render.h"
#include "zelda_sound.h"
#include "librecomp/helpers.hpp"
#include "../patches/input.h"
#include "../patches/graphics.h"
#include "../patches/sound.h"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/config.hpp"
#include "hle/rt64_lighting_instrumentation.h"
#include "json/json.hpp"

static_assert(sizeof(RecompAtmosphereOverride) == (11 * sizeof(uint32_t)));
static_assert(sizeof(RecompEnvironmentFog) == (40 * sizeof(uint32_t)));
static_assert((sizeof(RecompLightingGameSnapshot) % sizeof(uint32_t)) == 0);
static_assert((sizeof(RecompLightingBindingFrame) % sizeof(uint32_t)) == 0);

template <typename T>
static T copy_recomp_words(uint8_t* rdram, gpr source) {
    static_assert((sizeof(T) % sizeof(uint32_t)) == 0);
    T result{};
    uint32_t* words = reinterpret_cast<uint32_t*>(&result);
    for (size_t i = 0; i < (sizeof(T) / sizeof(uint32_t)); i++) {
        words[i] = MEM_W(i * sizeof(uint32_t), source);
    }
    return result;
}

static nlohmann::json lighting_vec3(const float values[3]) {
    return nlohmann::json::array({ values[0], values[1], values[2] });
}

static nlohmann::json lighting_vec3i(const s32 values[3]) {
    return nlohmann::json::array({ values[0], values[1], values[2] });
}

static nlohmann::json lighting_published_environment_json(const RecompLightingGameSnapshot& s) {
    if (!s.publishedEnvironmentAvailable) {
        return { { "status", "unavailable" }, { "reason", "environment_adapter_packet_not_published" } };
    }
    const RecompEnvironmentFog& e = s.publishedEnvironment;
    return {
        { "status", "observed" }, { "packet_schema", "RecompEnvironmentFog_v1" },
        { "valid", e.valid != 0 }, { "rgb_u24", e.rgb }, { "fog_near", e.fogNear }, { "z_far", e.zFar },
        { "sun_direction", { e.sunX, e.sunY, e.sunZ } },
        { "camera_position", { e.cameraX, e.cameraY, e.cameraZ } },
        { "view_direction", { e.viewX, e.viewY, e.viewZ } },
        { "reference_height", e.referenceHeight },
        { "weather", { { "outdoor", e.outdoor != 0 }, { "rain", e.rain != 0 }, { "snow", e.snow != 0 },
            { "storm", e.storm != 0 }, { "expanded_outdoor", e.expandedOutdoor != 0 } } },
        { "profile", {
            { "override_mask", e.atmosphereOverrideMask }, { "base_height_blend", e.baseHeightBlend },
            { "morning_height_blend", e.morningHeightBlend }, { "scale_height_fraction", e.scaleHeightFraction },
            { "density_variation", e.densityVariation }, { "directional_scattering", e.directionalScattering },
            { "saturated_fog_height_budget", e.saturatedFogHeightBudget },
            { "clear_air_far_transmittance", e.clearAirFarTransmittance },
            { "wet_air_far_transmittance", e.wetAirFarTransmittance }, { "water_influence", e.waterInfluence },
            { "ambient_rgb_u24", e.ambientRGB }, { "sky_fill_weight", e.skyFillWeight },
            { "primary_direction", { e.primaryDirection[0], e.primaryDirection[1], e.primaryDirection[2] } },
            { "primary_rgb_u24", e.primaryRGB },
            { "secondary_direction", { e.secondaryDirection[0], e.secondaryDirection[1], e.secondaryDirection[2] } },
            { "secondary_rgb_u24", e.secondaryRGB }, { "local_bounce_strength", e.localBounceStrength }
        } }
    };
}

static nlohmann::json lighting_game_json(const RecompLightingGameSnapshot& s) {
    nlohmann::json nodes = nlohmann::json::array();
    for (uint32_t i = 0; i < std::min<uint32_t>(s.nodeCount, RECOMP_LIGHTING_CAPTURE_MAX_NODES); i++) {
        const RecompLightingNodeSnapshot& node = s.nodes[i];
        nodes.push_back({
            { "ordinal", node.ordinal }, { "type", node.type },
            { "position_or_direction", { node.x, node.y, node.z } },
            { "radius", node.radius }, { "positive_range", node.radius > 0 },
            { "rgb_u24", node.rgb }, { "glow", node.glow != 0 },
            { "environment_directional_1", node.environment1 != 0 },
            { "environment_directional_2", node.environment2 != 0 },
            { "provenance", (node.environment1 || node.environment2) ? "environment_directional" : "unknown" }
        });
    }

    return {
        { "schema_version", s.schemaVersion }, { "status", "observed" },
        { "phase", s.phase == 0 ? "pre_draw" : "post_draw" },
        { "identity", { { "play_epoch", s.playEpoch }, { "gameplay_frame", s.gameplayFrame } } },
        { "location", {
            { "scene_id", s.sceneId }, { "scene_layer", s.sceneLayer }, { "saved_entrance", s.savedEntrance },
            { "current_spawn", s.curSpawn }, { "requested_next_entrance", s.nextEntrance },
            { "transition_trigger", s.transitionTrigger }, { "transition_type", s.transitionType },
            { "transition_mode", s.transitionMode }, { "current_room", s.currentRoom },
            { "previous_room", s.previousRoom }, { "room_load_status", s.roomLoadStatus },
            { "current_room_segment_valid", s.currentRoomSegmentValid != 0 },
            { "previous_room_segment_valid", s.previousRoomSegmentValid != 0 },
            { "current_room_enable_pos_lights", s.currentRoomEnablePosLights != 0 },
            { "current_room_behavior_1", s.currentRoomBehavior1 }, { "current_room_behavior_2", s.currentRoomBehavior2 }
        } },
        { "mode_cinematic", {
            { "active_gamestate", "PlayState" }, { "game_mode", s.gameMode }, { "cutscene_state", s.cutsceneState },
            { "cutscene_frame", s.cutsceneFrame }, { "cutscene_script_index", s.cutsceneScriptIndex },
            { "saved_cutscene_index", s.savedCutsceneIndex }, { "current_cutscene_id", s.currentCutsceneId },
            { "play_in_cutscene", s.playInCutscene != 0 }
        } },
        { "clocks_weather", {
            { "day_raw", s.dayRaw }, { "current_day", s.currentDay }, { "current_time", s.currentTime },
            { "skybox_time", s.skyboxTime }, { "scene_time_speed", s.sceneTimeSpeed },
            { "weather_mode", s.weatherMode }, { "storm_request", s.stormRequest }, { "storm_state", s.stormState },
            { "lightning_state", s.lightningState }, { "precipitation", s.precipitation }
        } },
        { "environment_selection", {
            { "light_mode", s.lightMode }, { "light_config", s.lightConfig },
            { "change_next_config", s.changeLightNextConfig }, { "change_enabled", s.changeLightEnabled != 0 },
            { "change_timer", s.changeLightTimer }, { "change_duration", s.changeDuration },
            { "blend_enabled", s.lightBlendEnabled != 0 }, { "light_setting", s.lightSetting },
            { "previous_light_setting", s.previousLightSetting }, { "setting_override", s.lightSettingOverride },
            { "blend_rate_override", s.lightBlendRateOverride }, { "blend", s.lightBlend },
            { "blend_override", s.lightBlendOverride }
        } },
        { "environment_values", {
            { "adjustments", s.adjustment }, { "resolved_settings", s.resolvedLightSettings },
            { "ambient_rgb_u24", s.ambientRGB }, { "fog_rgb_u24", s.fogRGB },
            { "fog_near", s.fogNear }, { "z_far", s.zFar },
            { "primary_direction", lighting_vec3i(s.primaryDirection) }, { "primary_rgb_u24", s.primaryRGB },
            { "secondary_direction", lighting_vec3i(s.secondaryDirection) }, { "secondary_rgb_u24", s.secondaryRGB },
            { "sun_position", lighting_vec3(s.sunPosition) }
        } },
        { "adapter_published_environment", lighting_published_environment_json(s) },
        { "spatial", {
            { "camera_eye", lighting_vec3(s.cameraEye) }, { "camera_at", lighting_vec3(s.cameraAt) },
            { "camera_up", lighting_vec3(s.cameraUp) }, { "player_position", lighting_vec3(s.playerPosition) },
            { "player_rotation", lighting_vec3i(s.playerRotation) }
        } },
        { "light_context", {
            { "status", s.nodeCycleDetected ? "malformed" : (s.nodeDropped ? "truncated" : (s.phase == 0 ? "observed" : "not_evaluated")) },
            { "reason", s.phase == 0 ? nullptr : nlohmann::json("post_phase_does_not_rescan") },
            { "observed_count", s.nodeCount }, { "dropped", s.nodeDropped },
            { "cycle_detected", s.nodeCycleDetected != 0 }, { "malformed", s.nodeMalformed != 0 },
            { "nodes", std::move(nodes) }
        } }
    };
}

extern "C" void recomp_update_inputs(uint8_t* rdram, recomp_context* ctx) {
    recomp::poll_inputs();
}

extern "C" void recomp_puts(uint8_t* rdram, recomp_context* ctx) {
    PTR(char) cur_str = _arg<0, PTR(char)>(rdram, ctx);
    u32 length = _arg<1, u32>(rdram, ctx);

    for (u32 i = 0; i < length; i++) {
        fputc(MEM_B(i, (gpr)cur_str), stdout);
    }
}

extern "C" void recomp_exit(uint8_t* rdram, recomp_context* ctx) {
    ultramodern::quit();
}

extern "C" void recomp_get_gyro_deltas(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);

    recomp::get_gyro_deltas(x_out, y_out);
}

extern "C" void recomp_get_mouse_deltas(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);

    recomp::get_mouse_deltas(x_out, y_out);
}

extern "C" void recomp_powf(uint8_t* rdram, recomp_context* ctx) {
    float a = _arg<0, float>(rdram, ctx);
    float b = ctx->f14.fl; //_arg<1, float>(rdram, ctx);

    _return(ctx, std::pow(a, b));
}

extern "C" void recomp_get_target_framerate(uint8_t* rdram, recomp_context* ctx) {
    int frame_divisor = _arg<0, u32>(rdram, ctx);

    _return(ctx, ultramodern::get_target_framerate(60 / frame_divisor));
}

extern "C" void recomp_get_window_resolution(uint8_t* rdram, recomp_context* ctx) {
    int width, height;
    recompui::get_window_size(width, height);

    gpr width_out = _arg<0, PTR(u32)>(rdram, ctx);
    gpr height_out = _arg<1, PTR(u32)>(rdram, ctx);

    MEM_W(0, width_out) = (u32)width;
    MEM_W(0, height_out) = (u32)height;
}

extern "C" void recomp_get_target_aspect_ratio(uint8_t* rdram, recomp_context* ctx) {
    ultramodern::renderer::GraphicsConfig graphics_config = ultramodern::renderer::get_graphics_config();
    float original = _arg<0, float>(rdram, ctx);
    int width, height;
    recompui::get_window_size(width, height);

    switch (graphics_config.ar_option) {
        case ultramodern::renderer::AspectRatio::Original:
        default:
            _return(ctx, original);
            return;
        case ultramodern::renderer::AspectRatio::Expand:
            _return(ctx, std::max(static_cast<float>(width) / height, original));
            return;
    }
}

extern "C" void recomp_get_targeting_mode(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, static_cast<int>(zelda64::get_targeting_mode()));
}

extern "C" void recomp_get_bgm_volume(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, zelda64::get_bgm_volume() / 100.0f);
}

extern "C" void recomp_get_low_health_beeps_enabled(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, static_cast<u32>(zelda64::get_low_health_beeps_enabled()));
}

extern "C" void recomp_time_us(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, static_cast<u32>(std::chrono::duration_cast<std::chrono::microseconds>(ultramodern::time_since_start()).count()));
}

extern "C" void recomp_get_autosave_enabled(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, static_cast<s32>(zelda64::get_autosave_mode() == zelda64::AutosaveMode::On));
}

extern "C" void recomp_load_overlays(uint8_t * rdram, recomp_context * ctx) {
    u32 rom = _arg<0, u32>(rdram, ctx);
    PTR(void) ram = _arg<1, PTR(void)>(rdram, ctx);
    u32 size = _arg<2, u32>(rdram, ctx);

    load_overlays(rom, ram, size);
}

extern "C" void recomp_high_precision_fb_enabled(uint8_t * rdram, recomp_context * ctx) {
    _return(ctx, static_cast<s32>(zelda64::renderer::RT64HighPrecisionFBEnabled()));
}

extern "C" void recomp_get_resolution_scale(uint8_t* rdram, recomp_context* ctx) {
    _return(ctx, ultramodern::get_resolution_scale());
}

extern "C" void recomp_get_inverted_axes(uint8_t* rdram, recomp_context* ctx) {
    s32* x_out = _arg<0, s32*>(rdram, ctx);
    s32* y_out = _arg<1, s32*>(rdram, ctx);

    zelda64::CameraInvertMode mode = zelda64::get_camera_invert_mode();

    *x_out = (mode == zelda64::CameraInvertMode::InvertX || mode == zelda64::CameraInvertMode::InvertBoth);
    *y_out = (mode == zelda64::CameraInvertMode::InvertY || mode == zelda64::CameraInvertMode::InvertBoth);
}

extern "C" void recomp_get_analog_inverted_axes(uint8_t* rdram, recomp_context* ctx) {
    s32* x_out = _arg<0, s32*>(rdram, ctx);
    s32* y_out = _arg<1, s32*>(rdram, ctx);

    zelda64::CameraInvertMode mode = zelda64::get_analog_camera_invert_mode();

    *x_out = (mode == zelda64::CameraInvertMode::InvertX || mode == zelda64::CameraInvertMode::InvertBoth);
    *y_out = (mode == zelda64::CameraInvertMode::InvertY || mode == zelda64::CameraInvertMode::InvertBoth);
}

extern "C" void recomp_get_analog_cam_enabled(uint8_t* rdram, recomp_context* ctx) {
    _return<s32>(ctx, zelda64::get_analog_cam_mode() == zelda64::AnalogCamMode::On);
}

extern "C" void recomp_get_camera_inputs(uint8_t* rdram, recomp_context* ctx) {
    float* x_out = _arg<0, float*>(rdram, ctx);
    float* y_out = _arg<1, float*>(rdram, ctx);

    // TODO expose this in the menu
    constexpr float radial_deadzone = 0.05f;

    float x, y;

    recomp::get_right_analog(&x, &y);

    float magnitude = sqrtf(x * x + y * y);

    if (magnitude < radial_deadzone) {
        *x_out = 0.0f;
        *y_out = 0.0f;
    }
    else {
        float x_normalized = x / magnitude;
        float y_normalized = y / magnitude;

        *x_out = x_normalized * ((magnitude - radial_deadzone) / (1 - radial_deadzone));
        *y_out = y_normalized * ((magnitude - radial_deadzone) / (1 - radial_deadzone));
    }
}

extern "C" void recomp_set_right_analog_suppressed(uint8_t* rdram, recomp_context* ctx) {
    s32 suppressed = _arg<0, s32>(rdram, ctx);

    recomp::set_right_analog_suppressed(suppressed);
}

extern "C" void recomp_set_environment_fog(uint8_t* rdram, recomp_context* ctx) {
    const gpr fog = _arg<0, PTR(u32)>(rdram, ctx);
    const auto read_float = [rdram, fog](size_t index) {
        union {
            u32 word;
            float value;
        } converted = { static_cast<u32>(MEM_W(index * sizeof(u32), fog)) };
        return converted.value;
    };
    const bool valid = MEM_W(0, fog) != 0;
    const u32 rgb = MEM_W(sizeof(u32), fog);
    const s32 fog_near = static_cast<s32>(MEM_W(2 * sizeof(u32), fog));
    const s32 z_far = static_cast<s32>(MEM_W(3 * sizeof(u32), fog));

    zelda64::renderer::set_environment_fog({
        .valid = valid,
        .red = static_cast<uint8_t>((rgb >> 16) & 0xFF),
        .green = static_cast<uint8_t>((rgb >> 8) & 0xFF),
        .blue = static_cast<uint8_t>(rgb & 0xFF),
        .fog_near = static_cast<int16_t>(fog_near),
        .z_far = static_cast<int16_t>(z_far),
        .sun_x = read_float(4),
        .sun_y = read_float(5),
        .sun_z = read_float(6),
        .camera_x = read_float(7),
        .camera_y = read_float(8),
        .camera_z = read_float(9),
        .view_x = read_float(10),
        .view_y = read_float(11),
        .view_z = read_float(12),
        .reference_height = read_float(13),
        .outdoor = MEM_W(14 * sizeof(u32), fog) != 0,
        .expanded_outdoor = MEM_W(18 * sizeof(u32), fog) != 0,
        .rain = static_cast<uint8_t>(MEM_W(15 * sizeof(u32), fog)),
        .snow = static_cast<uint8_t>(MEM_W(16 * sizeof(u32), fog)),
        .storm = MEM_W(17 * sizeof(u32), fog) != 0,
        .atmosphere_override_mask = MEM_W(19 * sizeof(u32), fog),
        .base_height_blend = read_float(20),
        .morning_height_blend = read_float(21),
        .scale_height_fraction = read_float(22),
        .density_variation = read_float(23),
        .directional_scattering = read_float(24),
        .saturated_fog_height_budget = read_float(25),
        .clear_air_far_transmittance = read_float(26),
        .wet_air_far_transmittance = read_float(27),
        .water_influence = read_float(28),
        .ambient_rgb = MEM_W(29 * sizeof(u32), fog),
        .sky_fill_weight = read_float(30),
        .primary_direction = { read_float(31), read_float(32), read_float(33) },
        .primary_rgb = MEM_W(34 * sizeof(u32), fog),
        .secondary_direction = { read_float(35), read_float(36), read_float(37) },
        .secondary_rgb = MEM_W(38 * sizeof(u32), fog),
        .local_bounce_strength = read_float(39),
    });
}

extern "C" void recomp_room_occluder_completion_enabled(uint8_t* rdram, recomp_context* ctx) {
    // Default on: the extra entries rasterize no pixels. ZELDA64RECOMP_ROOM_OCCLUDERS=0 restores
    // original submission for A/B qualification.
    static const bool enabled = []() {
        const char* value = std::getenv("ZELDA64RECOMP_ROOM_OCCLUDERS");
        return (value == nullptr) || (std::strcmp(value, "0") != 0);
    }();
    _return<u32>(ctx, enabled ? 1U : 0U);
}

extern "C" void recomp_lighting_capture_is_armed(uint8_t* rdram, recomp_context* ctx) {
    RT64::LightingInstrumentation& instrumentation = RT64::LightingInstrumentation::instance();
    _return<u32>(ctx, instrumentation.detailedCapture() ? 1U : (instrumentation.benchmarkCapture() ? 2U : 0U));
}

extern "C" void recomp_lighting_capture_begin(uint8_t* rdram, recomp_context* ctx) {
    if (!RT64::LightingInstrumentation::instance().armed()) {
        _return<u32>(ctx, 0);
        return;
    }
    const gpr address = _arg<0, PTR(u32)>(rdram, ctx);
    const RecompLightingGameSnapshot snapshot = copy_recomp_words<RecompLightingGameSnapshot>(rdram, address);
    if (snapshot.schemaVersion != RECOMP_LIGHTING_CAPTURE_SCHEMA) {
        _return<u32>(ctx, 0);
        return;
    }
    _return<u32>(ctx, RT64::LightingInstrumentation::instance().publishGamePre(lighting_game_json(snapshot).dump()));
}

extern "C" void recomp_lighting_capture_post(uint8_t* rdram, recomp_context* ctx) {
    if (!RT64::LightingInstrumentation::instance().armed()) {
        return;
    }
    const u32 token = _arg<0, u32>(rdram, ctx);
    const gpr address = _arg<1, PTR(u32)>(rdram, ctx);
    const RecompLightingGameSnapshot snapshot = copy_recomp_words<RecompLightingGameSnapshot>(rdram, address);
    if (snapshot.schemaVersion == RECOMP_LIGHTING_CAPTURE_SCHEMA) {
        RT64::LightingInstrumentation::instance().publishGamePost(token, lighting_game_json(snapshot).dump());
    }
}

extern "C" void recomp_lighting_capture_finalize(uint8_t* rdram, recomp_context* ctx) {
    if (!RT64::LightingInstrumentation::instance().detailedCapture()) {
        return;
    }
    const gpr address = _arg<0, PTR(u32)>(rdram, ctx);
    const RecompLightingBindingFrame frame = copy_recomp_words<RecompLightingBindingFrame>(rdram, address);
    if ((frame.schemaVersion != RECOMP_LIGHTING_CAPTURE_SCHEMA) || (frame.token == 0)) {
        return;
    }

    nlohmann::json attempts = nlohmann::json::array();
    std::array<uint32_t, 5> outcomes = {};
    const uint32_t attemptCount = std::min<uint32_t>(frame.attemptCount, RECOMP_LIGHTING_CAPTURE_MAX_ATTEMPTS);
    for (uint32_t i = 0; i < attemptCount; i++) {
        const RecompLightingBindAttempt& attempt = frame.attempts[i];
        if (attempt.outcome < outcomes.size()) outcomes[attempt.outcome]++;
        const char* outcome = "unknown";
        switch (attempt.outcome) {
        case 1: outcome = "bound_verified"; break;
        case 2: outcome = "bound_unverified"; break;
        case 3: outcome = "not_bound"; break;
        case 4: outcome = "unsupported_type"; break;
        }
        nlohmann::json parameters;
        if (!attempt.parametersObserved) {
            parameters = { { "status", "unavailable" }, { "reason", "parameters_not_observed" } };
        }
        else if (attempt.parameterKind == 1) {
            parameters = { { "status", "observed" }, { "kind", "point" },
                { "position", { attempt.x, attempt.y, attempt.z } }, { "radius", attempt.radius }, { "rgb_u24", attempt.rgb } };
        }
        else if (attempt.parameterKind == 2) {
            parameters = { { "status", "observed" }, { "kind", "directional" },
                { "direction", { attempt.direction[0], attempt.direction[1], attempt.direction[2] } },
                { "rgb_u24", attempt.directionRGB } };
        }
        else {
            parameters = { { "status", "unavailable" }, { "reason", "unsupported_parameter_kind" },
                { "kind_value", attempt.parameterKind } };
        }
        attempts.push_back({
            { "bind_ordinal", attempt.bindOrdinal }, { "attempt_ordinal", attempt.attemptOrdinal },
            { "type", attempt.type }, { "positional_mode", attempt.positionalMode != 0 },
            { "reference", attempt.refPresent ? nlohmann::json::array({ attempt.refPosition[0], attempt.refPosition[1], attempt.refPosition[2] }) : nlohmann::json(nullptr) },
            { "parameters", std::move(parameters) },
            { "initial_slots", attempt.initialSlots }, { "before_slots", attempt.beforeSlots }, { "after_slots", attempt.afterSlots },
            { "preexisting_full_slots", attempt.preexistingFullSlots != 0 },
            { "returned_slot", attempt.returnedSlot == UINT32_MAX ? nlohmann::json(nullptr) : nlohmann::json(attempt.returnedSlot) },
            { "owns_point", attempt.ownsPoint != 0 }, { "outcome", outcome },
            { "not_bound_reason", attempt.outcome == 3 ? nlohmann::json("not_bound_reason_unobserved") : nlohmann::json(nullptr) }
        });
    }

    nlohmann::json drawEvents = nlohmann::json::array();
    const uint32_t drawEventCount = std::min<uint32_t>(frame.drawEventCount, RECOMP_LIGHTING_CAPTURE_MAX_DRAW_EVENTS);
    for (uint32_t i = 0; i < drawEventCount; i++) {
        const RecompLightingDrawReceiptEvent& event = frame.drawEvents[i];
        drawEvents.push_back({
            { "draw_ordinal", event.drawOrdinal },
            { "bind_ordinal", event.bindOrdinal == UINT32_MAX ? nlohmann::json(nullptr) : nlohmann::json(event.bindOrdinal) },
            { "num_lights", event.numLights }, { "receipt_found", event.receiptFound != 0 },
            { "receipt_equal", event.receiptEqual != 0 }, { "receipt_consumed", event.receiptConsumed != 0 },
            { "receipt_reused_or_mismatched", event.receiptReusedOrMismatched != 0 },
            { "annotated_source_slots", event.annotatedSourceSlots }
        });
    }

    nlohmann::json bindingJson = {
        { "schema_version", frame.schemaVersion },
        { "status", frame.attemptDropped || frame.drawEventDropped || frame.emissionDropped ? "truncated" : "observed" },
        { "bind_count", frame.bindCount }, { "bind_dropped", frame.bindDropped },
        { "attempt_count", frame.attemptCount }, { "attempt_dropped", frame.attemptDropped },
        { "receipt", {
            { "capacity", frame.receiptCapacity }, { "created", frame.receiptCount }, { "overflow", frame.receiptOverflow },
            { "invalidated", frame.receiptInvalidated }, { "found", frame.receiptFound }, { "equal", frame.receiptEqual },
            { "consumed", frame.receiptConsumed }, { "reused_or_mismatched", frame.receiptReusedOrMismatched }
        } },
        { "outcome_aggregates", {
            { "unknown", outcomes[0] }, { "bound_verified", outcomes[1] }, { "bound_unverified", outcomes[2] },
            { "not_bound", outcomes[3] }, { "unsupported_type", outcomes[4] }
        } },
        { "attempts", std::move(attempts) },
        { "draw_events", { { "observed", frame.drawCount }, { "retained", frame.drawEventCount },
            { "dropped", frame.drawEventDropped }, { "events", std::move(drawEvents) } } },
        { "emissions", { { "observed", frame.emissionCount }, { "dropped", frame.emissionDropped } } }
    };

    std::vector<RT64::LightingAnnotationSidecar> sidecars;
    const uint32_t emissionCount = std::min<uint32_t>(frame.emissionCount, RECOMP_LIGHTING_CAPTURE_MAX_EMISSIONS);
    sidecars.reserve(emissionCount);
    for (uint32_t i = 0; i < emissionCount; i++) {
        const RecompLightingAnnotationEmission& source = frame.emissions[i];
        RT64::LightingAnnotationSidecar sidecar;
        sidecar.token = source.token;
        sidecar.rdramAddress = source.rdramAddress;
        sidecar.stream = source.stream;
        sidecar.slot = source.slot;
        sidecar.bindOrdinal = source.bindOrdinal;
        std::copy(std::begin(source.commandWords), std::end(source.commandWords), sidecar.commandWords.begin());
        std::copy(std::begin(source.payloadWords), std::end(source.payloadWords), sidecar.payloadWords.begin());
        sidecars.emplace_back(sidecar);
    }

    RT64::LightingInstrumentation::instance().finalizeBindings(frame.token, bindingJson.dump(), std::move(sidecars),
        frame.opaMarkerEmitted != 0, frame.xluMarkerEmitted != 0);
}
