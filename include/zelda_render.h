#ifndef __ZELDA_RENDER_H__
#define __ZELDA_RENDER_H__

#include <unordered_set>
#include <filesystem>
#include <cstdint>

#include "common/rt64_user_configuration.h"
#include "ultramodern/renderer_context.hpp"
#include "librecomp/mods.hpp"

namespace RT64 {
    struct Application;
}

namespace zelda64 {
    namespace renderer {
        inline const std::string special_option_texture_pack_enabled = "_recomp_texture_pack_enabled";

        struct EnvironmentFog {
            bool valid = false;
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;
            int16_t fog_near = 0;
            int16_t z_far = 0;
            float sun_x = 0.0f;
            float sun_y = 1.0f;
            float sun_z = 0.0f;
            float camera_x = 0.0f;
            float camera_y = 0.0f;
            float camera_z = 0.0f;
            float view_x = 0.0f;
            float view_y = 0.0f;
            float view_z = 1.0f;
            float reference_height = 0.0f;
            bool outdoor = false;
            bool expanded_outdoor = false;
            uint8_t rain = 0;
            uint8_t snow = 0;
            bool storm = false;
            uint32_t atmosphere_override_mask = 0;
            float base_height_blend = 0.22f;
            float morning_height_blend = 0.69f;
            float scale_height_fraction = 0.015f;
            float density_variation = 0.12f;
            float directional_scattering = 0.25f;
            float saturated_fog_height_budget = 0.03f;
            float clear_air_far_transmittance = 0.90f;
            float wet_air_far_transmittance = 0.65f;
            float water_influence = 0.0f;
        };

        void set_environment_fog(const EnvironmentFog& fog);
        EnvironmentFog get_environment_fog();

        class RT64Context final : public ultramodern::renderer::RendererContext {
        public:
            ~RT64Context() override;
            RT64Context(uint8_t *rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

            bool valid() override { return static_cast<bool>(app); }

            bool update_config(const ultramodern::renderer::GraphicsConfig &old_config, const ultramodern::renderer::GraphicsConfig &new_config) override;

            void enable_instant_present() override;
            void send_dl(const OSTask *task) override;
            void update_screen() override;
            void shutdown() override;
            uint32_t get_display_framerate() const override;
            float get_resolution_scale() const override;

        private:
            std::unique_ptr<RT64::Application> app;
            std::unordered_set<std::string> enabled_texture_packs;
            std::unordered_set<std::string> secondary_disabled_texture_packs;

            void check_texture_pack_actions();
        };

        std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context(uint8_t *rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

        RT64::UserConfiguration::Antialiasing RT64MaxMSAA();
        bool RT64SamplePositionsSupported();
        bool RT64HighPrecisionFBEnabled();

        void trigger_texture_pack_update();
        void enable_texture_pack(const recomp::mods::ModContext& context, const recomp::mods::ModHandle& mod);
        void disable_texture_pack(const recomp::mods::ModHandle& mod);
        void secondary_enable_texture_pack(const std::string& mod_id);
        void secondary_disable_texture_pack(const std::string& mod_id);

        // Texture pack enable option. Must be an enum with two options.
        // The first option is treated as disabled and the second option is treated as enabled.
        bool is_texture_pack_enable_config_option(const recomp::mods::ConfigOption& option, bool show_errors);
    }
}

#endif
