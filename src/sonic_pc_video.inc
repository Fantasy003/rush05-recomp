// SPDX-License-Identifier: GPL-3.0-or-later
enum class SonicScene {
    Other,
    Gameplay,
    BossBattle,
    StageIntro,
    StageReady,
    BossEntry,
    BossTransition,
    Results,
    Selection,
};

SonicScene sonic_current_scene() {
    // The supported USA revision uses a 256-entry ARM9 task pool.
    // Match both callbacks for tasks in shared overlays. Character select
    // keeps one destructor while swapping its update during menu transitions.
    constexpr struct {
        uint32_t first_update;
        uint32_t last_update;
        uint32_t destroy;
        SonicScene scene;
    } scene_tasks[] = {
        // This overlay task owns the model archive for the boss ascent/descent movie.
        {0x022e04bcu, 0x022e04bcu, 0x022e0588u, SonicScene::BossTransition},
        {0x02047ecdu, 0x02047ecdu, 0x02047d2du, SonicScene::Results},
        {0x02358525u, 0x02359625u, 0x02359d15u, SonicScene::Selection}, // character select
        {0x022d8c45u, 0x022d8c45u, 0x022d8c31u, SonicScene::Selection}, // main menu and Battle Play
        {0x022e83bdu, 0x022e83bdu, 0x022e8379u, SonicScene::Selection}, // zone map
        {0x022e129du, 0x022e129du, 0x022e0ee1u, SonicScene::Selection}, // act select
        {0x022eb5d1u, 0x022eb5d1u, 0x022ebe99u, SonicScene::Selection}, // Time Attack select
    };
    constexpr unsigned task_stride = 0x1c;
    std::array<uint8_t, 256 * task_stride> tasks;
    bus_debug_copy(9, 0x022b49a8u, tasks.data(), tasks.size());
    const auto read_task_word = [&](unsigned offset) {
        uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= uint32_t{tasks[offset + byte]} << (byte * 8);
        return value;
    };
    bool player = false;
    bool stage_intro = false;
    SonicScene overlay_scene = SonicScene::Other;
    for (unsigned task = 0; task < tasks.size(); task += task_stride) {
        const uint32_t update = read_task_word(task + 0x08);
        const bool stage_object = (update & ~1u) == 0x0206c3d0u;
        const bool intro_controller = update == 0x0205ec5cu;
        const auto* match = std::find_if(std::begin(scene_tasks), std::end(scene_tasks),
            [update](const auto& entry) {
                return entry.first_update <= update && update <= entry.last_update;
            });
        if (!stage_object && !intro_controller && match == std::end(scene_tasks)) continue;
        const uint32_t data = read_task_word(task + 0x10);
        if (data < 0x02000000u || data >= 0x02400000u || (data & 3u)) continue;
        if (stage_object) {
            player = true;
        } else if (intro_controller) {
            // The stage intro sets this flag in the same routine that
            // restores the player actions and starts the stage timer.
            if (data <= 0x023ffb74u && read_task_word(task + 0x0c) == 0x0205eb68u &&
                !(bus_debug_read8(9, data + 0x48bu) & 0x20u))
                stage_intro = true;
        } else if (read_task_word(task + 0x0c) == match->destroy) {
            if (match->scene == SonicScene::BossTransition) return match->scene;
            if (overlay_scene == SonicScene::Other) overlay_scene = match->scene;
        }
    }
    if (overlay_scene != SonicScene::Other) return overlay_scene;
    if (!stage_intro && !player) return SonicScene::Other;
    // Regular zones use acts 0 and 1 for the "On Your Mark" intro.
    const uint8_t act = bus_debug_read8(9, 0x022c457du);
    if (stage_intro) {
        if (act == 2) return SonicScene::BossEntry;
        if (act < 2 && bus_debug_read8(9, 0x022c457cu) < 7) return SonicScene::StageReady;
        return SonicScene::StageIntro;
    }
    return act == 2 ? SonicScene::BossBattle : SonicScene::Gameplay;
}

struct SonicFrameView {
    int focus;
    bool player;
    bool stack_screens;
    bool horizontal_screens;
    bool boss_hud;

    unsigned visible_lcds() const {
        return focus < 0 || boss_hud ? 3u : 1u << focus;
    }
};

SonicFrameView sonic_frame_view(bool separate, bool fit, bool menu_open, int main_lcd) {
    const SonicScene scene = sonic_current_scene();
    const bool boss = scene == SonicScene::BossBattle;
    const bool player = boss || scene == SonicScene::Gameplay;
    const bool game_paused = player && (bus_debug_read8(9, 0x022c4ef0u) & 1u) != 0;
    const bool force_dual = (fit || separate) && (menu_open || game_paused);
    // Engine A draws the regular-act start prompt and results on its routed LCD.
    const bool focus_scene = player || (!separate &&
        ((scene == SonicScene::StageReady && !menu_open) ||
         (fit && scene == SonicScene::Results)));
    return {
        sonic_pc_focus_screen(g_sonic_pc.screen_mode != 0, focus_scene && !force_dual, main_lcd),
        player,
        scene == SonicScene::BossTransition || (scene == SonicScene::Selection && !menu_open),
        !separate && !menu_open && g_sonic_pc.screen_mode != 0 && scene == SonicScene::BossEntry,
        separate && boss,
    };
}

#if defined(NDS_HAVE_SDL3)
#include "sonic_glove_cursor.inc"

std::array<SDL_DisplayID, 2> sonic_monitor_pair() {
    const SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    SDL_DisplayID secondary = 0;
    for (int i = 0; primary && i < count; ++i) {
        if (displays[i] != primary) {
            secondary = displays[i];
            break;
        }
    }
    SDL_free(displays);
    return {primary, secondary};
}

bool sonic_place_on_monitor(SDL_Window* window, SDL_DisplayID display) {
    const auto center = SDL_WINDOWPOS_CENTERED_DISPLAY(display);
    return SDL_SetWindowPosition(window, center, center) && SDL_SyncWindow(window) &&
           SDL_GetDisplayForWindow(window) == display;
}
#endif

bool sonic_create_presentation(NdsFrontendOptions& options, FrontendPresentation& p) {
#if defined(NDS_HAVE_SDL3)
    if (g_sonic_pc.dual_monitors) {
        const auto displays = sonic_monitor_pair();
        if (displays[1]) {
            auto separate_options = options;
            separate_options.screen_layout = NdsScreenLayout::Separate;
            // Both monitors can draw either native LCD as the active screen
            // changes. Fit-to-Screen scales those LCDs at presentation time.
            separate_options.adaptive_screens = 0;
            if (create_presentation(separate_options, p, false) &&
                sonic_place_on_monitor(p.windows[0], displays[0]) &&
                sonic_place_on_monitor(p.windows[1], displays[1])) {
                options = std::move(separate_options);
                p.monitor_lcds = sonic_monitor_lcds(g_sonic_pc.dual_monitors_inverted, -1);
                SDL_RaiseWindow(p.windows[0]);
                return true;
            }
            destroy_presentation(p);
            p = {};
        }
        std::fprintf(stderr, "[sonic-pc] Dual monitors unavailable; using one window\n");
    }
#endif
    options.screen_layout = NdsScreenLayout::Stacked;
    return create_presentation(options, p, false);
}

NdsFullscreenMode sonic_pc_fullscreen_mode(int window_mode, bool separate) {
    if (window_mode == 0) return NdsFullscreenMode::Off;
    // In the two-window layout both LCDs must stay visible as either window
    // gains focus. Keep each monitor at its desktop resolution and refresh.
    if (separate || window_mode == 2) return NdsFullscreenMode::Borderless;
    return NdsFullscreenMode::Exclusive;
}

bool sonic_apply_pc_video(FrontendPresentation& p, NdsFrontendOptions& options,
                          const SonicPcPreferences* previous = nullptr) {
    const auto mode = sonic_pc_fullscreen_mode(g_sonic_pc.window_mode, p.separate);
    const bool different_window = !previous ||
        sonic_pc_fullscreen_mode(previous->window_mode, p.separate) != mode;
    const bool different_size = !previous || previous->resolution != g_sonic_pc.resolution;
    const bool auto_orientation = previous && g_sonic_pc.resolution == 0 &&
        (previous->screen_mode != g_sonic_pc.screen_mode ||
         previous->aspect_mode != g_sonic_pc.aspect_mode);
    const int window_count = p.separate ? 2 : 1;
    for (int screen = 0; screen < window_count; ++screen) {
        SDL_Window* window = p.windows[screen];
        if (different_window || (mode == NdsFullscreenMode::Exclusive && different_size)) {
            if (!set_window_fullscreen(window, mode)) return false;
        }
        if (mode == NdsFullscreenMode::Off && (different_window || different_size || auto_orientation)) {
            int width, height;
            sonic_pc_window_pixels(width, height);
            if (p.separate && g_sonic_pc.resolution == 0) {
                width = p.screen_widths[screen] * kWindowScale;
                height = kScreenHeight * kWindowScale;
            }
#if defined(NDS_HAVE_SDL3)
            const SDL_DisplayID display = p.separate ? SDL_GetDisplayForWindow(window) : 0;
#endif
            if (!SDL_SetWindowBordered(window, true) ||
                !SDL_SetWindowSize(window, width, height)) return false;
#if defined(NDS_HAVE_SDL3)
            if (display && !sonic_place_on_monitor(window, display)) return false;
#endif
        }
    }
    if (g_sonic_pc.filter == 2) {
        for (int screen = 0; screen < 2; ++screen) {
            if (p.edge_textures[screen]) continue;
            p.edge_textures[screen] = SDL_CreateTexture(p.renderers[screen],
                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                p.screen_widths[screen] * 2, kScreenHeight * 2);
            if (!p.edge_textures[screen] ||
                !SDL_SetTextureScaleMode(p.edge_textures[screen],SDL_SCALEMODE_LINEAR)) {
                if (p.edge_textures[screen]) SDL_DestroyTexture(p.edge_textures[screen]);
                p.edge_textures[screen] = nullptr;
                return false;
            }
        }
    }
    for (auto* texture : p.textures) {
        if (texture && !SDL_SetTextureScaleMode(texture,
                g_sonic_pc.filter == 1 ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST)) return false;
    }
    options.fullscreen=mode;
    // Changing a view never writes into the emulated console or moves Sonic.
    // Mouse coordinates are re-established in the newly presented viewport.
    nds_set_touch(0,0,false);
    return true;
}

bool sonic_apply_internal_resolution(int preset, int previous) {
#if defined(NDS_HAVE_COMPUTE_RENDERER)
    auto height = [](int index) -> uint16_t {
        return index ? kSonicRasterHeights[index] : 0;
    };
    if (!preset) {
        nds_compute_host_stop();
        return nds_gpu3d_set_pc_raster_height(0);
    }
    if (!nds_gpu3d_set_pc_raster_height(height(preset))) return false;
    if (nds_compute_host_active()) {
        if (nds_compute_host_make_current() && nds_gpu3d_use_compute_renderer()) return true;
    } else if (nds_compute_host_start(nullptr, true) && nds_compute_host_active()) {
        return true;
    }
    nds_gpu3d_set_pc_raster_height(height(previous));
#else
    (void)previous;
#endif
    return preset == 0;
}

void sonic_upload_screen(FrontendPresentation& p, int screen, int lcd, const uint32_t* pixels,
                         int width, bool edge_smoothing, bool allow_raster) {
    p.raster_ready[screen] = false;
#if defined(NDS_HAVE_COMPUTE_RENDERER)
    NdsGpu2dPcRasterFrame layers{};
    const uint32_t raster = allow_raster ? nds_gpu3d_pc_raster_texture() : 0;
    if (raster && width == 256 && nds_gpu2d_pc_raster_frame(lcd, &layers)) {
        const int raster_height = kSonicRasterHeights[p.pc_raster_preset];
        const int raster_width = (width * raster_height + 96) / 192;
        auto* renderer = p.renderers[screen];
        std::array size{raster_width, raster_height};
#if defined(NDS_HAVE_SDL3)
        // The raster retains the selected resolution. Read back only the number
        // of pixels the LCD occupies, so a small viewport doesn't transfer 4K.
        SDL_FRect viewport{};
        const auto& lcd_rect = g_sonic_screen_layout.lcd[lcd];
        if (SDL_GetRenderLogicalPresentationRect(renderer, &viewport) &&
            viewport.w > 0 && viewport.h > 0) {
            size[0] = std::clamp(static_cast<int>(std::ceil(
                viewport.w * lcd_rect.w / g_sonic_screen_layout.width)), width, raster_width);
            size[1] = std::clamp(static_cast<int>(std::ceil(
                viewport.h * lcd_rect.h / g_sonic_screen_layout.height)), kScreenHeight, raster_height);
        }
#endif
        // Reduce the synchronous GL-to-SDL transfer before stepping down
        // the 3D raster itself. Resolve with a uniform reduction to preserve aspect.
        if (p.pc_governor_stage >= 1 && size[1] > 1080) {
            size[0] = std::max(width, size[0] * 1080 / size[1]);
            size[1] = 1080;
        }
        auto*& texture = p.raster_textures[screen];
        if (p.raster_sizes[screen] != size) {
            if (texture) SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING, size[0], size[1]);
            p.raster_sizes[screen] = texture ? size : std::array<int, 2>{};
        }
        if (texture) {
            SDL_SetTextureScaleMode(texture, g_sonic_pc.filter == 0
                ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
            void* destination = nullptr;
            int pitch = 0;
            if (SDL_LockTexture(texture, nullptr, &destination, &pitch)) {
                p.raster_ready[screen] = nds_compute_host_resolve_pc_raster(
                    lcd, layers, pixels, raster, size[0], size[1], destination, pitch);
                SDL_UnlockTexture(texture);
            }
            if (p.raster_ready[screen]) {
                if (!p.raster_success_logged) {
                    std::fprintf(stderr, "[sonic-pc] Displayed %dx%d 3D raster on LCD %d (transfer %dx%d)\n",
                                 raster_width, raster_height, lcd + 1, size[0], size[1]);
                    p.raster_success_logged = true;
                }
                return;
            }
        }
        if (!p.raster_failure_logged) {
            std::fprintf(stderr, "[sonic-pc] Could not present the 3D raster. Using native pixels.\n");
            p.raster_failure_logged = true;
        }
    }
#else
    (void)lcd;
    (void)allow_raster;
#endif
    if (edge_smoothing && p.edge_textures[screen] && p.screen_widths[screen] == width) {
        p.edge_pixels.resize(static_cast<size_t>(width) * kScreenHeight * 4);
        sonic_scale2x(pixels, width, kScreenHeight, p.edge_pixels.data());
        p.edge_ready[screen] = SDL_UpdateTexture(p.edge_textures[screen], nullptr,
            p.edge_pixels.data(), 2 * width * sizeof(uint32_t));
        if (p.edge_ready[screen]) return;
    }
    p.edge_ready[screen] = false;
    SDL_UpdateTexture(p.textures[screen], nullptr, pixels, width * sizeof(uint32_t));
}

SDL_Rect sonic_screen_rect(int screen) {
    const auto& lcd = g_sonic_screen_layout.lcd[screen];
    return SDL_Rect{lcd.x, lcd.y, lcd.w, lcd.h};
}

void sonic_draw_frame(SDL_Renderer* renderer, const SDL_Rect& screen) {
    if (g_sonic_pc.border != 2) return;
    const SDL_FRect edge{static_cast<float>(screen.x - 1), static_cast<float>(screen.y - 1),
                        static_cast<float>(screen.w + 2), static_cast<float>(screen.h + 2)};
    SDL_SetRenderDrawColor(renderer, 75, 82, 94, 255);
    SDL_RenderRect(renderer, &edge);
}
