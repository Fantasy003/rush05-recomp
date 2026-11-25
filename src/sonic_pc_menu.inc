// Sonic Rush PC settings. Included inside frontend.cpp's SDL namespace.
// SPDX-License-Identifier: GPL-3.0-or-later
constexpr const char* kRuntimeResumeKey = "system.resume";
constexpr const char* kRuntimeQuitKey = "system.quit";
struct RuntimeMenuState {
    bool* running = nullptr;
    bool close_requested = false;
    std::function<bool(const SonicPcPreferences&)> apply_display;
    std::function<bool()> supports_high_refresh;
    std::vector<RecompRuntimeUiItem> items;
    std::string settings_path;
    RecompRuntimeUi* ui = nullptr;
    int capture_kind = 0;
    unsigned capture_action = 0;
    bool capture_menu_control = false;
    bool game_language_changed = false;
    bool frame_rate_changed = false;
};
RuntimeMenuState* g_sonic_menu = nullptr;
const char* sonic_text(const char* english, const char* spanish) {
    return g_sonic_pc.menu_language == 1 ? spanish : english;
}
enum SonicMenuSection { Display, Keyboard, Gamepad, MenuKeyboard, MenuGamepad, Languages, System, SectionCount };
char g_sonic_sections[SectionCount][24]{};
char g_sonic_subtitle[24]{};
char g_sonic_menu_help[48]{};
char g_sonic_key_labels[12][72]{};
char g_sonic_pad_labels[12][72]{};
char g_sonic_key_ids[12][40]{};
char g_sonic_pad_ids[12][40]{};
char g_sonic_menu_key_labels[kSonicMenuActionCount][72]{};
char g_sonic_menu_pad_labels[kSonicMenuActionCount][72]{};
char g_sonic_menu_key_ids[kSonicMenuActionCount][32]{};
char g_sonic_menu_pad_ids[kSonicMenuActionCount][32]{};
constexpr const char* kSonicSpanishActions[12] = {
    "Saltar / Aceptar", "Truco / Atras", "Select", "Inicio / Pausa",
    "Mover a derecha", "Mover a izquierda", "Mirar arriba", "Agacharse / Abajo",
    "Impulso aereo / Riel", "Boton L", "Turbo", "Turbo alternativo"
};
constexpr const char* kSonicEnglishActions[12] = {
    "Jump / Confirm", "Trick / Back", "Select", "Start / Pause",
    "Move right", "Move left", "Look up", "Crouch / Down",
    "Air dash / Grind", "L button", "Boost", "Alt. boost"
};
constexpr unsigned kSonicGameplayOrder[] = {5,4,6,7,0,10,8,1,3,11,2,9};
constexpr const char* kSonicMenuEnglishActions[kSonicMenuActionCount] = {
    "Up", "Down", "Left", "Right", "Confirm", "Back"
};
constexpr const char* kSonicMenuSpanishActions[kSonicMenuActionCount] = {
    "Arriba", "Abajo", "Izquierda", "Derecha", "Aceptar", "Atras"
};
const char* const kSonicViews[] = {"NDS", "Concentrado"};
const char* kSonicAspectModes[2]{};
const char* const kSonicFrameRates[] = {"30 FPS", "60 FPS", "120 FPS"};
const char* kSonicWindows[3]{};
const char* kSonicResolutions[] = {"Auto", "640 x 480", "800 x 600",
    "1024 x 768", "1280 x 720", "1600 x 900", "1920 x 1080", "2560 x 1440", "3840 x 2160"};
const char* kSonicFilters[3]{};
const char* kSonicBorders[3]{};
const char* const kSonicLanguages[] = {"English", "Espanol"};

void sonic_pc_window_pixels(int& width,int& height) {
    static constexpr int sizes[9][2]={{0,0},{640,480},{800,600},{1024,768},
        {1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160}};
    width=sizes[g_sonic_pc.resolution][0];height=sizes[g_sonic_pc.resolution][1];
    if (width) return;
    if (g_sonic_pc.aspect_mode == 1) {
        width = 1280;
        height = 720;
        return;
    }
    width = g_sonic_pc.screen_mode ? 768 : 512;
    height = g_sonic_pc.screen_mode ? 576 : 768;
}
const char* sonic_pad_button_name(int button) {
    switch(button) {
        case SDL_GAMEPAD_BUTTON_SOUTH:return "A / Cruz";
        case SDL_GAMEPAD_BUTTON_EAST:return sonic_text("B / Circle","B / Circulo");
        case SDL_GAMEPAD_BUTTON_WEST:return sonic_text("X / Square","X / Cuadrado");
        case SDL_GAMEPAD_BUTTON_NORTH:return sonic_text("Y / Triangle","Y / Triangulo");
        case SDL_GAMEPAD_BUTTON_BACK:return "Back / Select";
        case SDL_GAMEPAD_BUTTON_START:return "Start / Options";
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:return "LB / L1";
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:return "RB / R1";
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:return sonic_text("Left stick","Stick izq.");
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:return sonic_text("Right stick","Stick der.");
        case SDL_GAMEPAD_BUTTON_DPAD_UP:return sonic_text("D-pad up","Cruceta arriba");
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:return sonic_text("D-pad down","Cruceta abajo");
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:return sonic_text("D-pad left","Cruceta izquierda");
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:return sonic_text("D-pad right","Cruceta derecha");
        default: { const char* s=SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));return s?s:"Otro"; }
    }
}
void sonic_refresh_binding_labels() {
    const auto& names = g_sonic_pc.menu_language ? kSonicSpanishActions : kSonicEnglishActions;
    for(unsigned i=0;i<12;++i) {
        const char* key=SDL_GetScancodeName(static_cast<SDL_Scancode>(g_sonic_pc.keys[i]));
        std::snprintf(g_sonic_key_labels[i],sizeof(g_sonic_key_labels[i]),"%s: %s",names[i],key&&*key?key:"?");
        std::snprintf(g_sonic_pad_labels[i],sizeof(g_sonic_pad_labels[i]),"%s: %s",names[i],sonic_pad_button_name(g_sonic_pc.buttons[i]));
    }
    const auto& menu_names = g_sonic_pc.menu_language ? kSonicMenuSpanishActions : kSonicMenuEnglishActions;
    for (unsigned i = 0; i < kSonicMenuActionCount; ++i) {
        const char* key = SDL_GetScancodeName(static_cast<SDL_Scancode>(g_sonic_pc.menu_keys[i]));
        std::snprintf(g_sonic_menu_key_labels[i], sizeof(g_sonic_menu_key_labels[i]),
                      "%s: %s", menu_names[i], key && *key ? key : "?");
        std::snprintf(g_sonic_menu_pad_labels[i], sizeof(g_sonic_menu_pad_labels[i]),
                      "%s: %s", menu_names[i], sonic_pad_button_name(g_sonic_pc.menu_buttons[i]));
    }
    const char* confirm_key = SDL_GetScancodeName(static_cast<SDL_Scancode>(
        g_sonic_pc.menu_keys[static_cast<unsigned>(SonicMenuAction::Confirm)]));
    const char* back_key = SDL_GetScancodeName(static_cast<SDL_Scancode>(
        g_sonic_pc.menu_keys[static_cast<unsigned>(SonicMenuAction::Back)]));
    std::snprintf(g_sonic_menu_help, sizeof(g_sonic_menu_help),
                  sonic_text("%.12s OPEN  %.12s BACK","%.12s ABRE  %.12s ATRAS"),
                  confirm_key && *confirm_key ? confirm_key : "?",
                  back_key && *back_key ? back_key : "?");
}
std::vector<RecompRuntimeUiItem> sonic_build_menu_items() {
    const char* sections[] = {
        sonic_text("Display","Pantalla"), sonic_text("Keyboard","Teclado"),
        sonic_text("Gamepad","Mando"), sonic_text("PC menu keys","Teclas del menu PC"),
        sonic_text("PC menu pad","Mando del menu PC"), sonic_text("Languages","Idiomas"),
        sonic_text("System","Sistema")
    };
    for (unsigned i = 0; i < SectionCount; ++i)
        std::snprintf(g_sonic_sections[i], sizeof(g_sonic_sections[i]), "%s", sections[i]);
    std::snprintf(g_sonic_subtitle, sizeof(g_sonic_subtitle), "%s", sonic_text("PC Settings","Opciones de PC"));
    kSonicAspectModes[0] = "Original";
    kSonicAspectModes[1] = sonic_text("Fit-to-Screen","Llenar pantalla");
    kSonicWindows[0] = sonic_text("Windowed","Ventana");
    kSonicWindows[1] = sonic_text("Fullscreen","Pantalla completa");
    kSonicWindows[2] = sonic_text("Borderless","Sin bordes");
    kSonicFilters[0] = sonic_text("Sharp","Nitido");
    kSonicFilters[1] = sonic_text("Soft","Suave");
    kSonicFilters[2] = sonic_text("Smooth edges","Bordes suaves");
    kSonicBorders[0] = sonic_text("Original","Original");
    kSonicBorders[1] = sonic_text("Black","Negro");
    kSonicBorders[2] = sonic_text("Graphite","Grafito");

    std::vector<RecompRuntimeUiItem> items;
    auto choice = [&](const char* key, SonicMenuSection section, const char* label,
                      const char* help, const auto& choices) {
        const auto count = std::size(choices);
        items.push_back({key,g_sonic_sections[section],label,help,RECOMP_RUNTIME_UI_CHOICE,
                         0,static_cast<int>(count)-1,1,choices,count,nullptr});
    };
    auto action = [&](const char* key, SonicMenuSection section, const char* label, const char* help) {
        items.push_back({key,g_sonic_sections[section],label,help,RECOMP_RUNTIME_UI_ACTION,0,0,0,nullptr,0,nullptr});
    };
    choice("sonic.screen_mode", Display, sonic_text("View","Modo"),
           sonic_text("Both screens or follow the action.","Dos pantallas o seguir la accion."), kSonicViews);
    choice("sonic.window_mode", Display, sonic_text("Window","Ventana"),
           sonic_text("Select how the window is shown.","Cambia el modo de ventana."), kSonicWindows);
    choice("sonic.resolution", Display, sonic_text("Resolution","Resolucion"),
           sonic_text("Borderless uses desktop resolution.","Sin bordes usa la del escritorio."), kSonicResolutions);
    choice("sonic.filter", Display, sonic_text("Scaling filter","Filtro"),
           sonic_text("Smooth edges softens pixel steps.","Suaviza las diagonales de pixeles."), kSonicFilters);
    choice("sonic.border", Display, sonic_text("Screen frame","Marco"),
           sonic_text("Uniform frame around the DS image.","Marco uniforme para la imagen."), kSonicBorders);
    choice("sonic.aspect_mode", Display, sonic_text("Aspect","Proporcion"),
           sonic_text("Fit fills the screen in Concentrado.","Concentrado llena en los niveles."), kSonicAspectModes);
    choice("sonic.frame_rate", Display, sonic_text("Frame rate","FPS"),
           sonic_text("120 FPS blends on 120 Hz displays.","120 FPS mezcla cuadros a 120 Hz."), kSonicFrameRates);
    for (unsigned i : kSonicGameplayOrder) {
        std::snprintf(g_sonic_key_ids[i],sizeof(g_sonic_key_ids[i]),"sonic.key.%u",i);
        action(g_sonic_key_ids[i], Keyboard, g_sonic_key_labels[i],
               sonic_text("Confirm, then press a key.","Acepta y pulsa una tecla."));
    }
    for (unsigned i : kSonicGameplayOrder) {
        std::snprintf(g_sonic_pad_ids[i],sizeof(g_sonic_pad_ids[i]),"sonic.pad.%u",i);
        action(g_sonic_pad_ids[i], Gamepad, g_sonic_pad_labels[i],
               sonic_text("Accept, then press a button.","Acepta y pulsa un boton."));
    }
    for (unsigned i = 0; i < kSonicMenuActionCount; ++i) {
        std::snprintf(g_sonic_menu_key_ids[i],sizeof(g_sonic_menu_key_ids[i]),"sonic.menu.key.%u",i);
        std::snprintf(g_sonic_menu_pad_ids[i],sizeof(g_sonic_menu_pad_ids[i]),"sonic.menu.pad.%u",i);
        action(g_sonic_menu_key_ids[i], MenuKeyboard, g_sonic_menu_key_labels[i],
               sonic_text("Only for the PC settings menu.","Solo para el menu de PC."));
        action(g_sonic_menu_pad_ids[i], MenuGamepad, g_sonic_menu_pad_labels[i],
               sonic_text("Only for the PC settings menu.","Solo para el menu de PC."));
    }
    choice("sonic.menu_language", Languages, sonic_text("PC menu","Menu de PC"),
           sonic_text("Changes the menu immediately.","Cambia el menu en el momento."), kSonicLanguages);
    choice("sonic.game_language", Languages, sonic_text("Game","Juego"),
           sonic_text("Applies when the game next starts.","Se aplica al volver a abrir el juego."), kSonicLanguages);
    action("sonic.reset_controls",System,sonic_text("Reset controls","Restablecer controles"),
           sonic_text("Restore the default controls.","Recupera los controles originales."));
    action(kRuntimeResumeKey,System,sonic_text("Resume","Reanudar"),
           sonic_text("Return to the game.","Volver al juego."));
    action(kRuntimeQuitKey,System,sonic_text("Quit","Salir"),
           sonic_text("Close Sonic Rush.","Cerrar Sonic Rush."));
    sonic_refresh_binding_labels();
    return items;
}
int* sonic_menu_preference(const char* key) {
    if (std::strcmp(key,"sonic.screen_mode")==0) return &g_sonic_pc.screen_mode;
    if (std::strcmp(key,"sonic.window_mode")==0) return &g_sonic_pc.window_mode;
    if (std::strcmp(key,"sonic.resolution")==0) return &g_sonic_pc.resolution;
    if (std::strcmp(key,"sonic.filter")==0) return &g_sonic_pc.filter;
    if (std::strcmp(key,"sonic.border")==0) return &g_sonic_pc.border;
    if (std::strcmp(key,"sonic.menu_language")==0) return &g_sonic_pc.menu_language;
    if (std::strcmp(key,"sonic.game_language")==0) return &g_sonic_pc.game_language;
    if (std::strcmp(key,"sonic.aspect_mode")==0) return &g_sonic_pc.aspect_mode;
    if (std::strcmp(key,"sonic.frame_rate")==0) return &g_sonic_pc.frame_rate;
    return nullptr;
}
int runtime_menu_get_value(void*,const RecompRuntimeUiItem* item,int* out) {
    if (!item || !out) return 0;
    const int* value = sonic_menu_preference(item->key);
    if (!value) return 0;
    *out = *value;
    return 1;
}
int runtime_menu_set_value(void* context,const RecompRuntimeUiItem* item,int value) {
    auto* state = static_cast<RuntimeMenuState*>(context);
    if (!state || !item || value < item->minimum || value > item->maximum) return 0;
    int* preference = sonic_menu_preference(item->key);
    if (!preference) return 0;
    const auto previous = g_sonic_pc;
    *preference = value;
    if (preference == &g_sonic_pc.frame_rate) {
        state->frame_rate_changed = true;
        return 1;
    }
    if (preference == &g_sonic_pc.menu_language) {
        // Replace labels in place so both the UI's items and section pointers
        // remain valid and the selected row stays where it is.
        const auto translated = sonic_build_menu_items();
        std::copy(translated.begin(), translated.end(), state->items.begin());
        return 1;
    }
    if (preference == &g_sonic_pc.game_language) {
        state->game_language_changed = true;
        return 1;
    }
    if (state->apply_display && !state->apply_display(previous)) {
        std::fprintf(stderr,"[sonic-pc] Video apply failed: %s\n",SDL_GetError());
        const auto failed = g_sonic_pc;
        g_sonic_pc = previous;
        state->apply_display(failed);
        if (state->ui) recomp_runtime_ui_set_status(state->ui,
            sonic_text("Display mode unavailable","Modo de pantalla no disponible"));
        return 0;
    }
    return 1;
}
void runtime_menu_save(void* context);

int runtime_menu_run_action(void* context,const RecompRuntimeUiItem* item) {
    auto* state=static_cast<RuntimeMenuState*>(context);if(!state||!item)return 0;
    if(std::strcmp(item->key,kRuntimeResumeKey)==0){state->close_requested=true;return 1;}
    if(std::strcmp(item->key,kRuntimeQuitKey)==0){if(state->running)*state->running=false;return 1;}
    if(std::strcmp(item->key,"sonic.reset_controls")==0) {
        const auto defaults = sonic_pc_defaults();
        g_sonic_pc.keys = defaults.keys;
        g_sonic_pc.buttons = defaults.buttons;
        g_sonic_pc.menu_keys = defaults.menu_keys;
        g_sonic_pc.menu_buttons = defaults.menu_buttons;
        sonic_refresh_binding_labels();
        runtime_menu_save(state);
        return 1;
    }
    if (std::strncmp(item->key,"sonic.",6) != 0) return 0;
    const bool pc_menu_binding = std::strncmp(item->key,"sonic.menu.",11) == 0;
    const char* binding = item->key + (pc_menu_binding ? 11 : 6);
    const bool key = std::strncmp(binding,"key.",4) == 0;
    if (key || std::strncmp(binding,"pad.",4) == 0) {
        state->capture_kind = key ? 1 : 2;
        state->capture_menu_control = pc_menu_binding;
        state->capture_action = static_cast<unsigned>(std::strtoul(binding + 4,nullptr,10));
        return 1;
    }
    return 0;
}
int runtime_menu_is_enabled(void*,const RecompRuntimeUiItem*) {return 1;}
void runtime_menu_save(void* context) {
    auto* state=static_cast<RuntimeMenuState*>(context);if(!state)return;
    const bool restart_notice = state->game_language_changed;
    const bool refresh_notice = state->frame_rate_changed && g_sonic_pc.frame_rate == 2 &&
                                state->supports_high_refresh && !state->supports_high_refresh();
    state->game_language_changed = false;
    state->frame_rate_changed = false;
    const bool saved = sonic_pc_save(state->settings_path);
    if (!state->ui) return;
    if (!saved) recomp_runtime_ui_set_status(state->ui,sonic_text("Could not save settings","No se pudieron guardar las opciones"));
    else if (restart_notice) recomp_runtime_ui_set_status(state->ui,sonic_text("Applies next launch","Se aplica al reabrir"));
    else if (refresh_notice) recomp_runtime_ui_set_status(state->ui,sonic_text("120 Hz needed. Using 60 FPS.","Faltan 120 Hz. Se usan 60 FPS."));
    else recomp_runtime_ui_set_status(state->ui,sonic_text("Saved","Guardado"));
}
bool sonic_capture_binding(RuntimeMenuState& state,bool keyboard,int code) {
    if(!state.capture_kind)return false;
    if((keyboard && code==SDL_SCANCODE_ESCAPE) || (!keyboard && code==SDL_GAMEPAD_BUTTON_GUIDE)) {
        state.capture_kind=0;
        if(state.ui)recomp_runtime_ui_set_status(state.ui,sonic_text("Change canceled","Cambio cancelado"));
        return true;
    }
    if((keyboard?1:2)!=state.capture_kind)return true;
    const bool ok = state.capture_menu_control
        ? (keyboard ? sonic_pc_bind_menu_key(state.capture_action, code)
                    : sonic_pc_bind_menu_button(state.capture_action, code))
        : (keyboard ? sonic_pc_bind_key(state.capture_action, code)
                    : sonic_pc_bind_button(state.capture_action, code));
    if(!ok) {if(state.ui)recomp_runtime_ui_set_status(state.ui,sonic_text("Reserved. Choose another.","Entrada reservada. Elegi otra."));return true;}
    state.capture_kind=0;sonic_refresh_binding_labels();runtime_menu_save(&state);
    return true;
}

// The USA game owns a 256-task pool; each task is 0x1c bytes. Only follow
// its live stage-object update tasks, rather than mistaking a menu's 3D logo
// or character preview for active gameplay. Reads stay in main RAM, without IO.
bool sonic_player_task_active() {
    auto word=[](uint32_t address) {
        uint32_t result=0;
        for(unsigned b=0;b<4;++b)result|=uint32_t{bus_debug_read8(9,address+b)}<<(b*8);
        return result;
    };
    for(unsigned i=0;i<256;++i) {
        const uint32_t node=0x022b49a8u+i*0x1cu;
        if((word(node+8)&~1u)!=0x0206c3d0u)continue;
        const uint32_t data=word(node+0x10);
        if(data>=0x02000000u && data<0x02400000u && !(data&3u))return true;
    }
    return false;
}
