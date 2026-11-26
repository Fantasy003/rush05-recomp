// SPDX-License-Identifier: GPL-3.0-or-later
enum class SonicScene {
    Other,
    Gameplay,
    Results,
    Selection,
};

SonicScene sonic_current_scene() {
    const auto read_word = [](uint32_t address) {
        uint32_t word = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            word |= uint32_t{bus_debug_read8(9, address + byte)} << (byte * 8);
        return word;
    };

    // The supported USA revision uses a 256-entry ARM9 task pool.
    // Match both callbacks for tasks in shared overlays. Character select
    // keeps one destructor while swapping its update during menu transitions.
    constexpr struct {
        uint32_t first_update;
        uint32_t last_update;
        uint32_t destroy;
        SonicScene scene;
    } scene_tasks[] = {
        {0x02047ecdu, 0x02047ecdu, 0x02047d2du, SonicScene::Results},
        {0x02358525u, 0x02359625u, 0x02359d15u, SonicScene::Selection}, // character select
        {0x022d8c45u, 0x022d8c45u, 0x022d8c31u, SonicScene::Selection}, // main menu and Battle Play
        {0x022e83bdu, 0x022e83bdu, 0x022e8379u, SonicScene::Selection}, // zone map
        {0x022e129du, 0x022e129du, 0x022e0ee1u, SonicScene::Selection}, // act select
        {0x022eb5d1u, 0x022eb5d1u, 0x022ebe99u, SonicScene::Selection}, // Time Attack select
    };
    SonicScene scene = SonicScene::Other;
    for (unsigned i = 0; i < 256; ++i) {
        const uint32_t task = 0x022b49a8u + i * 0x1cu;
        const uint32_t update = read_word(task + 0x08);
        const bool stage_object = (update & ~1u) == 0x0206c3d0u;
        const auto* match = std::find_if(std::begin(scene_tasks), std::end(scene_tasks),
            [update](const auto& entry) {
                return entry.first_update <= update && update <= entry.last_update;
            });
        if (!stage_object && match == std::end(scene_tasks)) continue;
        const uint32_t data = read_word(task + 0x10);
        if (data < 0x02000000u || data >= 0x02400000u || (data & 3u)) continue;
        if (stage_object) {
            scene = SonicScene::Gameplay;
        } else if (read_word(task + 0x0c) == match->destroy) {
            return match->scene;
        }
    }
    return scene;
}

bool sonic_apply_pc_video(FrontendPresentation& p, NdsFrontendOptions& options,
                          const SonicPcPreferences* previous = nullptr) {
    const NdsFullscreenMode mode=g_sonic_pc.window_mode==1?NdsFullscreenMode::Exclusive:
        g_sonic_pc.window_mode==2?NdsFullscreenMode::Borderless:NdsFullscreenMode::Off;
    const bool different_window = !previous || previous->window_mode != g_sonic_pc.window_mode;
    const bool different_size = !previous || previous->resolution != g_sonic_pc.resolution;
    const bool auto_orientation = previous && g_sonic_pc.resolution == 0 &&
        (previous->screen_mode != g_sonic_pc.screen_mode ||
         previous->aspect_mode != g_sonic_pc.aspect_mode);
    if (different_window || (mode == NdsFullscreenMode::Exclusive && different_size)) {
        if (!set_window_fullscreen(p.windows[0],mode)) return false;
    }
    if (mode == NdsFullscreenMode::Off && (different_window || different_size || auto_orientation)) {
        int width, height;
        sonic_pc_window_pixels(width,height);
        if (!SDL_SetWindowBordered(p.windows[0],true) ||
            !SDL_SetWindowSize(p.windows[0],width,height)) return false;
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

void sonic_upload_screen(FrontendPresentation& p, int screen, const uint32_t* pixels,
                         int width, bool edge_smoothing) {
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
