#include <atomic>
#include <cstdlib>
#include <cstdio>
#include "ultramodern/config.hpp"
#include "zelda_debug.h"
#include "librecomp/helpers.hpp"
#include "../patches/input.h"

std::atomic<uint16_t> pending_warp = 0xFFFF;
std::atomic<uint32_t> pending_set_time = 0xFFFF;

void zelda64::do_warp(int area, int scene, int entrance) {
    const zelda64::SceneWarps game_scene = zelda64::game_warps[area].scenes[scene];
    int game_scene_index = game_scene.index;
    pending_warp.store(((game_scene_index & 0xFF) << 8) | ((entrance & 0x0F) << 4));
}

extern "C" void recomp_get_pending_warp(uint8_t* rdram, recomp_context* ctx) {
    // Return the current warp value and reset it.
    _return(ctx, pending_warp.exchange(0xFFFF));
}

void zelda64::set_time(uint8_t day, uint8_t hour, uint8_t minute) {
    pending_set_time.store((day << 16) | (uint16_t(hour) << 8) | minute);
}

extern "C" void recomp_get_pending_set_time(uint8_t* rdram, recomp_context* ctx) {
    // One-shot reproducible lighting setup through the existing developer action.
    static bool launchTimeChecked = false;
    if (!launchTimeChecked && _arg<0, int32_t>(rdram, ctx) != 0) {
        launchTimeChecked = true;
        if (ultramodern::renderer::get_graphics_config().developer_mode) {
            if (const char* value = std::getenv("ZELDA64RECOMP_DEV_TIME")) {
                unsigned day, hour, minute;
                if (std::sscanf(value, "%u,%u,%u", &day, &hour, &minute) == 3 && day >= 1 && day <= 3 && hour < 24 && minute < 60) {
                    zelda64::set_time(day, hour, minute);
                    // Optional one-shot entrance through the existing warp action.
                    if (const char* entrance = std::getenv("ZELDA64RECOMP_DEV_ENTRANCE")) {
                        char* end = nullptr;
                        unsigned long code = std::strtoul(entrance, &end, 0);
                        if (*entrance && end && !*end && code < 0xFFFF && (code & 15) == 0)
                            pending_warp.store(static_cast<uint16_t>(code));
                    }
                    fprintf(stderr, "[Dev time] Day %u, %02u:%02u\n", day, hour, minute);
                }
            }
        }
    }
    // Return the current set time value and reset it.
    _return(ctx, pending_set_time.exchange(0xFFFF));
}
