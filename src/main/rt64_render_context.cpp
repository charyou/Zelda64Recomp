#include <memory>
#include <cstring>
#include <variant>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <mutex>
#include <string_view>

#define HLSL_CPU
#include "hle/rt64_application.h"
#include "hle/rt64_workload_queue.h"
#include "rt64_render_hooks.h"
#include "overloaded.h"

#include "ultramodern/ultramodern.hpp"
#include "ultramodern/config.hpp"

#include "zelda_render.h"
#include "recomp_ui.h"
#include "concurrentqueue.h"

static RT64::UserConfiguration::Antialiasing device_max_msaa = RT64::UserConfiguration::Antialiasing::None;
static bool sample_positions_supported = false;
static bool high_precision_fb_enabled = false;

namespace {
    std::mutex environment_fog_mutex;
    zelda64::renderer::EnvironmentFog environment_fog;

    RT64::FogMode fog_mode_from_environment(RT64::FogMode fallback) {
        const char* mode = std::getenv("ZELDA64RECOMP_FOG_MODE");
        if (mode == nullptr) {
            return fallback;
        }

        if (std::string_view(mode) == "original") {
            return RT64::FogMode::Original;
        }

        if (std::string_view(mode) == "faithful") {
            return RT64::FogMode::FaithfulPerPixel;
        }

        if (std::string_view(mode) == "atmospheric") {
            return RT64::FogMode::Atmospheric;
        }

        return fallback;
    }

    const char* fog_mode_name(RT64::FogMode mode) {
        switch (mode) {
        case RT64::FogMode::FaithfulPerPixel:
            return "Faithful Per-Pixel";
        case RT64::FogMode::Atmospheric:
            return "Atmospheric";
        default:
            return "Original";
        }
    }
}

void zelda64::renderer::set_environment_fog(const EnvironmentFog& fog) {
    std::lock_guard lock(environment_fog_mutex);
    environment_fog = fog;
}

zelda64::renderer::EnvironmentFog zelda64::renderer::get_environment_fog() {
    std::lock_guard lock(environment_fog_mutex);
    return environment_fog;
}

static uint8_t DMEM[0x1000];
static uint8_t IMEM[0x1000];

struct TexturePackEnableAction {
    std::string mod_id;
};

struct TexturePackDisableAction {
    std::string mod_id;
};

struct TexturePackSecondaryEnableAction {
    std::string mod_id;
};

struct TexturePackSecondaryDisableAction {
    std::string mod_id;
};

struct TexturePackUpdateAction {
};

using TexturePackAction = std::variant<TexturePackEnableAction, TexturePackDisableAction, TexturePackSecondaryEnableAction, TexturePackSecondaryDisableAction, TexturePackUpdateAction>;

static moodycamel::ConcurrentQueue<TexturePackAction> texture_pack_action_queue;

unsigned int MI_INTR_REG = 0;

unsigned int DPC_START_REG = 0;
unsigned int DPC_END_REG = 0;
unsigned int DPC_CURRENT_REG = 0;
unsigned int DPC_STATUS_REG = 0;
unsigned int DPC_CLOCK_REG = 0;
unsigned int DPC_BUFBUSY_REG = 0;
unsigned int DPC_PIPEBUSY_REG = 0;
unsigned int DPC_TMEM_REG = 0;

void dummy_check_interrupts() {}

RT64::UserConfiguration::Antialiasing compute_max_supported_aa(plume::RenderSampleCounts bits) {
    if (bits & plume::RenderSampleCount::Bits::COUNT_2) {
        if (bits & plume::RenderSampleCount::Bits::COUNT_4) {
            if (bits & plume::RenderSampleCount::Bits::COUNT_8) {
                return RT64::UserConfiguration::Antialiasing::MSAA8X;
            }
            return RT64::UserConfiguration::Antialiasing::MSAA4X;
        }
        return RT64::UserConfiguration::Antialiasing::MSAA2X;
    };
    return RT64::UserConfiguration::Antialiasing::None;
}

RT64::UserConfiguration::AspectRatio to_rt64(ultramodern::renderer::AspectRatio option) {
    switch (option) {
        case ultramodern::renderer::AspectRatio::Original:
            return RT64::UserConfiguration::AspectRatio::Original;
        case ultramodern::renderer::AspectRatio::Expand:
            return RT64::UserConfiguration::AspectRatio::Expand;
        case ultramodern::renderer::AspectRatio::Manual:
            return RT64::UserConfiguration::AspectRatio::Manual;
        case ultramodern::renderer::AspectRatio::OptionCount:
            return RT64::UserConfiguration::AspectRatio::OptionCount;
    }
}

RT64::UserConfiguration::Antialiasing to_rt64(ultramodern::renderer::Antialiasing option) {
    switch (option) {
        case ultramodern::renderer::Antialiasing::None:
            return RT64::UserConfiguration::Antialiasing::None;
        case ultramodern::renderer::Antialiasing::MSAA2X:
            return RT64::UserConfiguration::Antialiasing::MSAA2X;
        case ultramodern::renderer::Antialiasing::MSAA4X:
            return RT64::UserConfiguration::Antialiasing::MSAA4X;
        case ultramodern::renderer::Antialiasing::MSAA8X:
            return RT64::UserConfiguration::Antialiasing::MSAA8X;
        case ultramodern::renderer::Antialiasing::OptionCount:
            return RT64::UserConfiguration::Antialiasing::OptionCount;
    }
}

RT64::UserConfiguration::RefreshRate to_rt64(ultramodern::renderer::RefreshRate option) {
    switch (option) {
        case ultramodern::renderer::RefreshRate::Original:
            return RT64::UserConfiguration::RefreshRate::Original;
        case ultramodern::renderer::RefreshRate::Display:
            return RT64::UserConfiguration::RefreshRate::Display;
        case ultramodern::renderer::RefreshRate::Manual:
            return RT64::UserConfiguration::RefreshRate::Manual;
        case ultramodern::renderer::RefreshRate::OptionCount:
            return RT64::UserConfiguration::RefreshRate::OptionCount;
    }
}

RT64::UserConfiguration::InternalColorFormat to_rt64(ultramodern::renderer::HighPrecisionFramebuffer option) {
    switch (option) {
        case ultramodern::renderer::HighPrecisionFramebuffer::Off:
            return RT64::UserConfiguration::InternalColorFormat::Standard;
        case ultramodern::renderer::HighPrecisionFramebuffer::On:
            return RT64::UserConfiguration::InternalColorFormat::High;
        case ultramodern::renderer::HighPrecisionFramebuffer::Auto:
            return RT64::UserConfiguration::InternalColorFormat::Automatic;
        case ultramodern::renderer::HighPrecisionFramebuffer::OptionCount:
            return RT64::UserConfiguration::InternalColorFormat::OptionCount;
    }
}

RT64::FogMode to_rt64(ultramodern::renderer::FogMode option) {
    switch (option) {
        case ultramodern::renderer::FogMode::FaithfulPerPixel:
            return RT64::FogMode::FaithfulPerPixel;
        case ultramodern::renderer::FogMode::Atmospheric:
            return RT64::FogMode::Atmospheric;
        case ultramodern::renderer::FogMode::Original:
        case ultramodern::renderer::FogMode::OptionCount:
            return RT64::FogMode::Original;
    }

    return RT64::FogMode::Original;
}

void set_application_user_config(RT64::Application* application, const ultramodern::renderer::GraphicsConfig& config) {
    switch (config.res_option) {
        default:
        case ultramodern::renderer::Resolution::Auto:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::WindowIntegerScale;
            application->userConfig.downsampleMultiplier = 1;
            break;
        case ultramodern::renderer::Resolution::Original:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
            application->userConfig.resolutionMultiplier = std::max(config.ds_option, 1);
            application->userConfig.downsampleMultiplier = std::max(config.ds_option, 1);
            break;
        case ultramodern::renderer::Resolution::Original2x:
            application->userConfig.resolution = RT64::UserConfiguration::Resolution::Manual;
            application->userConfig.resolutionMultiplier = 2.0 * std::max(config.ds_option, 1);
            application->userConfig.downsampleMultiplier = std::max(config.ds_option, 1);
            break;
    }

    switch (config.hr_option) {
        default:
        case ultramodern::renderer::HUDRatioMode::Original:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Original;
            break;
        case ultramodern::renderer::HUDRatioMode::Clamp16x9:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Manual;
            application->userConfig.extAspectTarget = 16.0/9.0;
            break;
        case ultramodern::renderer::HUDRatioMode::Full:
            application->userConfig.extAspectRatio = RT64::UserConfiguration::AspectRatio::Expand;
            break;
    }

    application->userConfig.aspectRatio = to_rt64(config.ar_option);
    application->userConfig.antialiasing = to_rt64(config.msaa_option);
    application->userConfig.refreshRate = to_rt64(config.rr_option);
    application->userConfig.refreshRateTarget = config.rr_manual_value;
    application->userConfig.internalColorFormat = to_rt64(config.hpfb_option);
    application->userConfig.displayBuffering = RT64::UserConfiguration::DisplayBuffering::Triple;
    application->setFogMode(to_rt64(config.fog_option));
    if (application->workloadQueue) {
        application->workloadQueue->rtShadows = config.rt_shadows;
        application->workloadQueue->rtAO = config.rt_ao;
        application->workloadQueue->rtEnvironmentFill = config.rt_environment_fill;
        application->workloadQueue->aoRadius = config.rt_ao_radius;
        application->workloadQueue->aoStrength = config.rt_ao_strength;
        application->workloadQueue->environmentRadius = config.rt_environment_radius;
        application->workloadQueue->environmentStrength = config.rt_environment_strength;
        application->workloadQueue->spatialSamples = config.rt_spatial_samples;
        application->workloadQueue->spatialBias = config.rt_spatial_bias;
        application->workloadQueue->authoredFillBudget = config.rt_authored_fill_budget;
        application->workloadQueue->ambientFloor = config.rt_ambient_floor;
    }
}

ultramodern::renderer::SetupResult map_setup_result(RT64::Application::SetupResult rt64_result) {
    switch (rt64_result) {
        case RT64::Application::SetupResult::Success:
            return ultramodern::renderer::SetupResult::Success;
        case RT64::Application::SetupResult::DynamicLibrariesNotFound:
            return ultramodern::renderer::SetupResult::DynamicLibrariesNotFound;
        case RT64::Application::SetupResult::InvalidGraphicsAPI:
            return ultramodern::renderer::SetupResult::InvalidGraphicsAPI;
        case RT64::Application::SetupResult::GraphicsAPINotFound:
            return ultramodern::renderer::SetupResult::GraphicsAPINotFound;
        case RT64::Application::SetupResult::GraphicsDeviceNotFound:
            return ultramodern::renderer::SetupResult::GraphicsDeviceNotFound;
    }

    fprintf(stderr, "Unhandled `RT64::Application::SetupResult` ?\n");
    assert(false);
    std::exit(EXIT_FAILURE);
}

ultramodern::renderer::GraphicsApi map_graphics_api(RT64::UserConfiguration::GraphicsAPI api) {
    switch (api) {
        case RT64::UserConfiguration::GraphicsAPI::D3D12:
            return ultramodern::renderer::GraphicsApi::D3D12;
        case RT64::UserConfiguration::GraphicsAPI::Vulkan:
            return ultramodern::renderer::GraphicsApi::Vulkan;
        case RT64::UserConfiguration::GraphicsAPI::Metal:
            return ultramodern::renderer::GraphicsApi::Metal;
        case RT64::UserConfiguration::GraphicsAPI::Automatic:
            return ultramodern::renderer::GraphicsApi::Auto;
    }

    fprintf(stderr, "Unhandled `RT64::UserConfiguration::GraphicsAPI` ?\n");
    assert(false);
    std::exit(EXIT_FAILURE);
}

zelda64::renderer::RT64Context::RT64Context(uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool debug) {
    static unsigned char dummy_rom_header[0x40];
    recompui::set_render_hooks();

    // Set up the RT64 application core fields.
    RT64::Application::Core appCore{};
#if defined(_WIN32)
    appCore.window = window_handle.window;
#elif defined(__linux__) || defined(__ANDROID__)
    appCore.window = window_handle;
#elif defined(__APPLE__)
    appCore.window.window = window_handle.window;
    appCore.window.view = window_handle.view;
#endif

    appCore.checkInterrupts = dummy_check_interrupts;

    appCore.HEADER = dummy_rom_header;
    appCore.RDRAM = rdram;
    appCore.DMEM = DMEM;
    appCore.IMEM = IMEM;

    appCore.MI_INTR_REG = &MI_INTR_REG;

    appCore.DPC_START_REG = &DPC_START_REG;
    appCore.DPC_END_REG = &DPC_END_REG;
    appCore.DPC_CURRENT_REG = &DPC_CURRENT_REG;
    appCore.DPC_STATUS_REG = &DPC_STATUS_REG;
    appCore.DPC_CLOCK_REG = &DPC_CLOCK_REG;
    appCore.DPC_BUFBUSY_REG = &DPC_BUFBUSY_REG;
    appCore.DPC_PIPEBUSY_REG = &DPC_PIPEBUSY_REG;
    appCore.DPC_TMEM_REG = &DPC_TMEM_REG;

    ultramodern::renderer::ViRegs* vi_regs = ultramodern::renderer::get_vi_regs();

    appCore.VI_STATUS_REG = &vi_regs->VI_STATUS_REG;
    appCore.VI_ORIGIN_REG = &vi_regs->VI_ORIGIN_REG;
    appCore.VI_WIDTH_REG = &vi_regs->VI_WIDTH_REG;
    appCore.VI_INTR_REG = &vi_regs->VI_INTR_REG;
    appCore.VI_V_CURRENT_LINE_REG = &vi_regs->VI_V_CURRENT_LINE_REG;
    appCore.VI_TIMING_REG = &vi_regs->VI_TIMING_REG;
    appCore.VI_V_SYNC_REG = &vi_regs->VI_V_SYNC_REG;
    appCore.VI_H_SYNC_REG = &vi_regs->VI_H_SYNC_REG;
    appCore.VI_LEAP_REG = &vi_regs->VI_LEAP_REG;
    appCore.VI_H_START_REG = &vi_regs->VI_H_START_REG;
    appCore.VI_V_START_REG = &vi_regs->VI_V_START_REG;
    appCore.VI_V_BURST_REG = &vi_regs->VI_V_BURST_REG;
    appCore.VI_X_SCALE_REG = &vi_regs->VI_X_SCALE_REG;
    appCore.VI_Y_SCALE_REG = &vi_regs->VI_Y_SCALE_REG;

    // Set up the RT64 application configuration fields.
    RT64::ApplicationConfiguration appConfig;
    appConfig.useConfigurationFile = false;

    // Create the RT64 application.
    app = std::make_unique<RT64::Application>(appCore, appConfig);

    // Set initial user config settings based on the current settings.
    auto& cur_config = ultramodern::renderer::get_graphics_config();
    set_application_user_config(app.get(), cur_config);
    app->userConfig.developerMode = debug;
    const RT64::FogMode initialFogMode = fog_mode_from_environment(to_rt64(cur_config.fog_option));
    app->setFogMode(initialFogMode);
    const char* lightingOverride = std::getenv("ZELDA64RECOMP_LIGHTING");
    const bool perPixelLighting = !lightingOverride || std::strcmp(lightingOverride, "original") != 0;
    app->perPixelLighting.store(perPixelLighting, std::memory_order_relaxed);
    fprintf(stderr, "RT64 lighting: %s (F1 Lighting tab; ZELDA64RECOMP_LIGHTING=original restores legacy)\n",
        perPixelLighting ? "Per-pixel" : "Original");
    fprintf(stdout, "RT64 fog mode: %s (Graphics menu; F5 temporarily cycles modes)\n", fog_mode_name(initialFogMode));
    // Force gbi depth branches to prevent LODs from kicking in.
    app->enhancementConfig.f3dex.forceBranch = true;
    // Scale LODs based on the output resolution.
    app->enhancementConfig.textureLOD.scale = true;
    // Pick an API if the user has set an override.
    switch (cur_config.api_option) {
        case ultramodern::renderer::GraphicsApi::D3D12:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::D3D12;
            break;
        case ultramodern::renderer::GraphicsApi::Vulkan:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Vulkan;
            break;
        case ultramodern::renderer::GraphicsApi::Metal:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Metal;
            break;
        case ultramodern::renderer::GraphicsApi::Auto:
            app->userConfig.graphicsAPI = RT64::UserConfiguration::GraphicsAPI::Automatic;
            break;
    }

    // Set up the RT64 application.
    uint32_t thread_id = 0;
#ifdef _WIN32
    thread_id = window_handle.thread_id;
#endif
    setup_result = map_setup_result(app->setup(thread_id));
    // Get the API that RT64 chose.
    chosen_api = map_graphics_api(app->chosenGraphicsAPI);
    if (setup_result != ultramodern::renderer::SetupResult::Success) {
        app = nullptr;
        return;
    }

    // Persistent setting, with an explicit launch override for finite diagnostics.
    const char* fillOverride = std::getenv("RT64_RT_ENVIRONMENT_FILL");
    app->workloadQueue->rtEnvironmentFill = fillOverride ? std::strcmp(fillOverride, "1") == 0 : cur_config.rt_environment_fill;
    app->workloadQueue->aoRadius = cur_config.rt_ao_radius;
    app->workloadQueue->aoStrength = cur_config.rt_ao_strength;
    app->workloadQueue->environmentRadius = cur_config.rt_environment_radius;
    app->workloadQueue->environmentStrength = cur_config.rt_environment_strength;
    app->workloadQueue->spatialSamples = cur_config.rt_spatial_samples;
    app->workloadQueue->spatialBias = cur_config.rt_spatial_bias;
    app->workloadQueue->authoredFillBudget = cur_config.rt_authored_fill_budget;
    app->workloadQueue->ambientFloor = cur_config.rt_ambient_floor;
    const char* aoOverride = std::getenv("RT64_RT_AO");
    app->workloadQueue->rtAO = aoOverride ? std::strcmp(aoOverride, "1") == 0 : cur_config.rt_ao;
    const char* shadowOverride = std::getenv("RT64_RT_SHADOWS");
    app->workloadQueue->rtShadows = shadowOverride ? std::strcmp(shadowOverride, "1") == 0 : cur_config.rt_shadows;

    // Same inspector as F1, available when OS function-key injection is unreliable.
    if (const char* inspector = std::getenv("ZELDA64RECOMP_DEV_INSPECTOR")) {
        if (std::strcmp(inspector, "1") == 0) {
            app->processDeveloperShortcut(RT64::Application::DeveloperShortcut::Inspector);
        }
    }

    // Same presentation switch as F3, available without OS keyboard injection.
    // A new application starts with viewRDRAM=false; apply this once after setup.
    if (const char* native = std::getenv("ZELDA64RECOMP_DEV_NATIVE")) {
        const bool use_native = std::strcmp(native, "1") == 0;
        if (use_native) {
            app->processDeveloperShortcut(RT64::Application::DeveloperShortcut::ViewRDRAM);
        }
        else {
            fprintf(stderr, "[Dev render] Rejected ZELDA64RECOMP_DEV_NATIVE: expected 1; retaining enhanced presentation.\n");
        }
        const char* api_name = "Unknown";
        switch (chosen_api) {
        case ultramodern::renderer::GraphicsApi::D3D12: api_name = "D3D12"; break;
        case ultramodern::renderer::GraphicsApi::Vulkan: api_name = "Vulkan"; break;
        case ultramodern::renderer::GraphicsApi::Metal: api_name = "Metal"; break;
        case ultramodern::renderer::GraphicsApi::Auto: api_name = "Auto"; break;
        default: break;
        }
        fprintf(stderr, "[Dev render] API=%s; configured fog=%s; presentation=%s.\n",
            api_name, fog_mode_name(initialFogMode), use_native ? "Native/RDRAM" : "Enhanced");
        fflush(stderr);
    }

    // Set the application's fullscreen state.
    app->setFullScreen(cur_config.wm_option == ultramodern::renderer::WindowMode::Fullscreen);

    // Check if the selected device actually supports MSAA sample positions and MSAA for for the formats that will be used
    // and downgrade the configuration accordingly.
    if (app->device->getCapabilities().sampleLocations) {
        plume::RenderSampleCounts color_sample_counts = app->device->getSampleCountsSupported(plume::RenderFormat::R8G8B8A8_UNORM);
        plume::RenderSampleCounts depth_sample_counts = app->device->getSampleCountsSupported(plume::RenderFormat::D32_FLOAT);
        plume::RenderSampleCounts common_sample_counts = color_sample_counts & depth_sample_counts;
        device_max_msaa = compute_max_supported_aa(common_sample_counts);
        sample_positions_supported = true;
    }
    else {
        device_max_msaa = RT64::UserConfiguration::Antialiasing::None;
        sample_positions_supported = false;
    }

    high_precision_fb_enabled = app->shaderLibrary->usesHDR;
}

zelda64::renderer::RT64Context::~RT64Context() = default;

void zelda64::renderer::RT64Context::send_dl(const OSTask* task) {
    check_texture_pack_actions();
    const EnvironmentFog environment = get_environment_fog();
    RT64::AtmosphereParameters atmosphere{};
    atmosphere.valid = environment.valid;
    atmosphere.environmentFill = {
        ((environment.ambient_rgb >> 16) & 255) / 255.0f,
        ((environment.ambient_rgb >> 8) & 255) / 255.0f,
        (environment.ambient_rgb & 255) / 255.0f,
        std::clamp(environment.sky_fill_weight, 0.0f, 1.0f) };
    atmosphere.fogColor = hlslpp::float4(
        environment.red / 255.0f,
        environment.green / 255.0f,
        environment.blue / 255.0f,
        1.0f);
    atmosphere.fogNear = static_cast<float>(environment.fog_near);
    atmosphere.zFar = static_cast<float>(environment.z_far);
    atmosphere.sunDirection = hlslpp::float3(environment.sun_x, environment.sun_y, environment.sun_z);
    atmosphere.cameraPosition = hlslpp::float3(environment.camera_x, environment.camera_y, environment.camera_z);
    atmosphere.viewDirection = hlslpp::float3(environment.view_x, environment.view_y, environment.view_z);
    atmosphere.referenceHeight = environment.reference_height;
    atmosphere.outdoorStrength = environment.outdoor ? 1.0f : 0.0f;
    atmosphere.conservativeOutdoorStrength = atmosphere.outdoorStrength;
    // Automatic skyless world views retain a little distance haze. The shader's
    // far-distance scaling gives at least a 10,000-unit haze length indoors,
    // so short rooms remain clear even when their authored zFar is small;
    // authored fog still wins via max optical depth. Explicit mod OFF stays off.
    const bool outdoorOverride = (environment.atmosphere_override_mask & RT64::AtmosphereOverrideOutdoor) != 0;
    const float indoorStrength = 0.15f * std::clamp(atmosphere.zFar / 10000.0f, 0.0f, 1.0f);
    atmosphere.expandedOutdoorStrength = environment.expanded_outdoor ? 1.0f : (outdoorOverride ? 0.0f : indoorStrength);
    atmosphere.overrideMask = environment.atmosphere_override_mask;
    atmosphere.baseHeightBlend = environment.base_height_blend;
    atmosphere.morningHeightBlend = environment.morning_height_blend;
    atmosphere.scaleHeightFraction = environment.scale_height_fraction;
    atmosphere.densityVariation = environment.density_variation;
    atmosphere.directionalScattering = environment.directional_scattering;
    atmosphere.saturatedFogHeightBudget = environment.saturated_fog_height_budget;
    atmosphere.clearAirFarTransmittance = environment.clear_air_far_transmittance;
    atmosphere.wetAirFarTransmittance = environment.wet_air_far_transmittance;
    const float rainStrength = std::clamp(environment.rain / 60.0f, 0.0f, 1.0f);
    const float snowStrength = std::clamp(environment.snow / 128.0f, 0.0f, 1.0f) * 0.65f;
    atmosphere.weatherStrength = std::max(rainStrength, snowStrength);
    // This generic medium parameter describes wet air; precipitation is not its
    // only source. Missing/custom water semantics retain the dry-air fallback.
    if (std::isfinite(environment.water_influence)) {
        atmosphere.weatherStrength = std::max(atmosphere.weatherStrength,
            std::clamp(environment.water_influence, 0.0f, 1.0f));
    }
    if (environment.storm) {
        atmosphere.weatherStrength = std::max(atmosphere.weatherStrength, 0.75f);
    }
    // MM itself treats the final 50 fog-position units below ENV_FOGNEAR_MAX (996)
    // as its continuous environmental fog influence for sun glare and lens flare.
    // Reuse that authored signal here instead of inventing scene-specific thresholds.
    atmosphere.fogStrength = std::clamp((996.0f - atmosphere.fogNear) / 50.0f, 0.0f, 1.0f);
    app->setAtmosphere(atmosphere);
    app->state->rsp->reset();
    app->interpreter->loadUCodeGBI(task->t.ucode & 0x3FFFFFF, task->t.ucode_data & 0x3FFFFFF, true);
    app->processDisplayLists(app->core.RDRAM, task->t.data_ptr & 0x3FFFFFF, 0, true);
}

void zelda64::renderer::RT64Context::update_screen() {
    app->updateScreen();
}

void zelda64::renderer::RT64Context::shutdown() {
    if (app != nullptr) {
        app->end();
    }
}

bool zelda64::renderer::RT64Context::update_config(const ultramodern::renderer::GraphicsConfig& old_config, const ultramodern::renderer::GraphicsConfig& new_config) {
    if (old_config == new_config) {
        return false;
    }

    if (new_config.wm_option != old_config.wm_option) {
        app->setFullScreen(new_config.wm_option == ultramodern::renderer::WindowMode::Fullscreen);
    }

    set_application_user_config(app.get(), new_config);

    if (new_config.fog_option != old_config.fog_option) {
        fprintf(stdout, "RT64 fog mode: %s\n", fog_mode_name(app->getFogMode()));
    }

    app->updateUserConfig(true);

    if (new_config.msaa_option != old_config.msaa_option) {
        app->updateMultisampling();
    }
    return true;
}

void zelda64::renderer::RT64Context::enable_instant_present() {
    // Enable the present early presentation mode for minimal latency.
    app->enhancementConfig.presentation.mode = RT64::EnhancementConfiguration::Presentation::Mode::PresentEarly;

    app->updateEnhancementConfig();
}

uint32_t zelda64::renderer::RT64Context::get_display_framerate() const {
    return app->presentQueue->ext.sharedResources->swapChainRate;
}

float zelda64::renderer::RT64Context::get_resolution_scale() const {
    constexpr int ReferenceHeight = 240;
    switch (app->userConfig.resolution) {
        case RT64::UserConfiguration::Resolution::WindowIntegerScale:
            if (app->sharedQueueResources->swapChainHeight > 0) {
                return std::max(float((app->sharedQueueResources->swapChainHeight + ReferenceHeight - 1) / ReferenceHeight), 1.0f);
            }
            else {
                return 1.0f;
            }
        case RT64::UserConfiguration::Resolution::Manual:
            return float(app->userConfig.resolutionMultiplier);
        case RT64::UserConfiguration::Resolution::Original:
        default:
            return 1.0f;
    }
}

void zelda64::renderer::RT64Context::check_texture_pack_actions() {
    bool packs_changed = false;
    TexturePackAction cur_action;
    while (texture_pack_action_queue.try_dequeue(cur_action)) {
        std::visit(overloaded{
            [&](TexturePackDisableAction &to_disable) {
                enabled_texture_packs.erase(to_disable.mod_id);
                packs_changed = true;
            },
            [&](TexturePackEnableAction &to_enable) {
                enabled_texture_packs.insert(to_enable.mod_id);
                packs_changed = true;
            },
            [&](TexturePackSecondaryDisableAction &to_override_disable) {
                secondary_disabled_texture_packs.insert(to_override_disable.mod_id);
                packs_changed = true;
            },
            [&](TexturePackSecondaryEnableAction &to_override_enable) {
                secondary_disabled_texture_packs.erase(to_override_enable.mod_id);
                packs_changed = true;
            },
            [&](TexturePackUpdateAction &) {
                packs_changed = true;
            }
        }, cur_action);
    }

    // If any packs were disabled, unload all packs and load all the active ones.
    if (packs_changed) {
        // Sort the enabled texture packs in reverse order so that earlier ones override later ones.
        std::vector<std::string> sorted_texture_packs{};
        sorted_texture_packs.reserve(enabled_texture_packs.size());
        for (const std::string& mod : enabled_texture_packs) {
            if (!secondary_disabled_texture_packs.contains(mod)) {
                sorted_texture_packs.emplace_back(mod);
            }
        }

        std::sort(sorted_texture_packs.begin(), sorted_texture_packs.end(),
            [](const std::string& lhs, const std::string& rhs) {
                return recomp::mods::get_mod_order_index(lhs) > recomp::mods::get_mod_order_index(rhs);
            }
        );

        // Build the path list from the sorted mod list.
        std::vector<RT64::ReplacementDirectory> replacement_directories;
        replacement_directories.reserve(enabled_texture_packs.size());
        for (const std::string &mod_id : sorted_texture_packs) {
            replacement_directories.emplace_back(RT64::ReplacementDirectory(recomp::mods::get_mod_filename(mod_id)));
        }

        if (!replacement_directories.empty()) {
            app->textureCache->loadReplacementDirectories(replacement_directories);
        }
        else {
            app->textureCache->clearReplacementDirectories();
        }
    }
}

RT64::UserConfiguration::Antialiasing zelda64::renderer::RT64MaxMSAA() {
    return device_max_msaa;
}

std::unique_ptr<ultramodern::renderer::RendererContext> zelda64::renderer::create_render_context(uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    return std::make_unique<zelda64::renderer::RT64Context>(rdram, window_handle, developer_mode);
}

bool zelda64::renderer::RT64SamplePositionsSupported() {
    return sample_positions_supported;
}

bool zelda64::renderer::RT64HighPrecisionFBEnabled() {
    return high_precision_fb_enabled;
}

void zelda64::renderer::trigger_texture_pack_update() {
    texture_pack_action_queue.enqueue(TexturePackUpdateAction{});
}

void zelda64::renderer::enable_texture_pack(const recomp::mods::ModContext& context, const recomp::mods::ModHandle& mod) {
    texture_pack_action_queue.enqueue(TexturePackEnableAction{mod.manifest.mod_id});

    // Check for the texture pack enabled config option.
    const recomp::mods::ConfigSchema& config_schema = context.get_mod_config_schema(mod.manifest.mod_id);
    auto find_it = config_schema.options_by_id.find(zelda64::renderer::special_option_texture_pack_enabled);
    if (find_it != config_schema.options_by_id.end()) {
        const recomp::mods::ConfigOption& config_option = config_schema.options[find_it->second];

        if (is_texture_pack_enable_config_option(config_option, false)) {
            recomp::mods::ConfigValueVariant value_variant = context.get_mod_config_value(mod.manifest.mod_id, config_option.id);
            uint32_t value;
            if (uint32_t* value_ptr = std::get_if<uint32_t>(&value_variant)) {
                value = *value_ptr;
            }
            else {
                value = 0;
            }

            if (value) {
                zelda64::renderer::secondary_enable_texture_pack(mod.manifest.mod_id);
            }
            else {
                zelda64::renderer::secondary_disable_texture_pack(mod.manifest.mod_id);
            }
        }
    }
}

void zelda64::renderer::disable_texture_pack(const recomp::mods::ModHandle& mod) {
    texture_pack_action_queue.enqueue(TexturePackDisableAction{mod.manifest.mod_id});
}

void zelda64::renderer::secondary_enable_texture_pack(const std::string& mod_id) {
    texture_pack_action_queue.enqueue(TexturePackSecondaryEnableAction{mod_id});
}

void zelda64::renderer::secondary_disable_texture_pack(const std::string& mod_id) {
    texture_pack_action_queue.enqueue(TexturePackSecondaryDisableAction{mod_id});
}


// HD texture enable option. Must be an enum with two options.
// The first option is treated as disabled and the second option is treated as enabled.
bool zelda64::renderer::is_texture_pack_enable_config_option(const recomp::mods::ConfigOption& option, bool show_errors) {
    if (option.id == zelda64::renderer::special_option_texture_pack_enabled) {
        if (option.type != recomp::mods::ConfigOptionType::Enum) {
            if (show_errors) {
                recompui::message_box(("Mod has the special config option id for enabling an HD texture pack (\"" + zelda64::renderer::special_option_texture_pack_enabled + "\"), but the config option is not an enum.").c_str());
            }
            return false;
        }

        const recomp::mods::ConfigOptionEnum &option_enum = std::get<recomp::mods::ConfigOptionEnum>(option.variant);
        if (option_enum.options.size() != 2) {
            if (show_errors) {
                recompui::message_box(("Mod has the special config option id for enabling an HD texture pack (\"" + zelda64::renderer::special_option_texture_pack_enabled + "\"), but the config option doesn't have exactly 2 values.").c_str());
            }
            return false;
        }

        return true;
    }
    return false;
}
