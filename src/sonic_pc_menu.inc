// Sonic Rush PC settings. Included inside frontend.cpp's SDL namespace.
// SPDX-License-Identifier: GPL-3.0-or-later
constexpr const char* kRuntimeResumeKey = "system.resume";
constexpr const char* kRuntimeQuitKey = "system.quit";
struct RuntimeMenuState {
    bool* running = nullptr;
    bool close_requested = false;
    std::function<bool(const SonicPcPreferences&)> apply_display;
    std::function<bool()> supports_high_refresh;
    std::function<bool(int, int)> apply_internal_resolution;
    std::vector<RecompRuntimeUiItem> items;
    std::string settings_path;
    RecompRuntimeUi* ui = nullptr;
    int capture_kind = 0;
    unsigned capture_action = 0;
    bool capture_menu_control = false;
    bool game_language_changed = false;
    bool frame_rate_changed = false;
    bool dual_monitors_changed = false;
    bool battle_changed = false;
};
RuntimeMenuState* g_sonic_menu = nullptr;
const char* sonic_text(const char* english, const char* spanish) {
    return g_sonic_pc.menu_language == 1 ? spanish : english;
}
enum SonicMenuSection { Display, Keyboard, Gamepad, MenuKeyboard, MenuGamepad, Languages, Gameplay, BattlePlay, System, SectionCount };
char g_sonic_sections[SectionCount][24]{};
char g_sonic_subtitle[24]{};
char g_sonic_menu_help[48]{};
char g_sonic_text_help[48]{};
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
const char* const kSonicViews[] = {"NDS", "Focus"};
const char* kSonicAspectModes[2]{};
const char* const kSonicFrameRates[] = {"30 FPS", "60 FPS", "120 FPS"};
const char* kSonicWindows[3]{};
const char* kSonicResolutions[] = {"Auto", "640 x 480", "800 x 600",
    "1024 x 768", "1280 x 720", "1600 x 900", "1920 x 1080", "2560 x 1440", "3840 x 2160"};
const char* kSonicInternalResolutions[kSonicRasterHeights.size()] = {
    nullptr, "240p", "480p", "560p", "720p", "1080p", "1440p", "2160p"
};
const char* kSonicFilters[3]{};
const char* kSonicBorders[3]{};
const char* const kSonicLanguages[] = {"English", "Espanol"};
const char* kSonicOffOnChoices[2]{};
const char* kSonicBattleRoles[3]{};

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
        sonic_text("Gameplay","Jugabilidad"),
        sonic_text("Battle Play","Modo batalla"),
        sonic_text("System","Sistema")
    };
    for (unsigned i = 0; i < SectionCount; ++i)
        std::snprintf(g_sonic_sections[i], sizeof(g_sonic_sections[i]), "%s", sections[i]);
    std::snprintf(g_sonic_subtitle, sizeof(g_sonic_subtitle), "%s", sonic_text("PC Settings","Opciones de PC"));
    std::snprintf(g_sonic_text_help, sizeof(g_sonic_text_help), "%s",
        sonic_text("Enter saves. Esc cancels.","Enter guarda. Esc cancela."));
    kSonicAspectModes[0] = "Original";
    kSonicAspectModes[1] = sonic_text("Fit-to-Screen","Llenar pantalla");
    kSonicWindows[0] = sonic_text("Windowed","Ventana");
    kSonicWindows[1] = sonic_text("Fullscreen","Pantalla completa");
    kSonicWindows[2] = sonic_text("Borderless","Sin bordes");
    kSonicInternalResolutions[0] = sonic_text("Native (192p)", "Nativa (192p)");
    kSonicFilters[0] = sonic_text("Sharp","Nitido");
    kSonicFilters[1] = sonic_text("Soft","Suave");
    kSonicFilters[2] = sonic_text("Smooth edges","Bordes suaves");
    kSonicBorders[0] = sonic_text("Original","Original");
    kSonicBorders[1] = sonic_text("Black","Negro");
    kSonicBorders[2] = sonic_text("Graphite","Grafito");
    kSonicOffOnChoices[0] = sonic_text("Off","No");
    kSonicOffOnChoices[1] = sonic_text("On","Si");
    kSonicBattleRoles[0] = sonic_text("Offline","Sin red");
    kSonicBattleRoles[1] = sonic_text("Host","Crear sala");
    kSonicBattleRoles[2] = sonic_text("Join","Unirse");

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
           sonic_text("Dual-monitor fullscreen uses desktop modes.",
                      "Con dos monitores, completa es sin bordes."), kSonicWindows);
    choice("sonic.resolution", Display, sonic_text("Resolution","Resolucion"),
           sonic_text("Borderless uses desktop resolution.","Sin bordes usa la del escritorio."), kSonicResolutions);
    choice("sonic.internal_resolution", Display, sonic_text("Internal resolution","Resolucion interna"),
           sonic_text("3D rendering. Higher settings need a faster GPU.",
                      "Render 3D. Los valores altos requieren mas GPU."), kSonicInternalResolutions);
    choice("sonic.filter", Display, sonic_text("Scaling filter","Filtro"),
           sonic_text("Smooth edges softens pixel steps.","Suaviza las diagonales de pixeles."), kSonicFilters);
    choice("sonic.border", Display, sonic_text("Screen frame","Marco"),
           sonic_text("Uniform frame around the DS image.","Marco uniforme para la imagen."), kSonicBorders);
    choice("sonic.aspect_mode", Display, sonic_text("Aspect","Proporcion"),
           sonic_text("Fit fills each monitor in dual mode.","Llenar ajusta la imagen a cada monitor."), kSonicAspectModes);
    choice("sonic.frame_rate", Display, sonic_text("Frame rate","FPS"),
           sonic_text("120 FPS blends on 120 Hz displays.","120 FPS mezcla cuadros a 120 Hz."), kSonicFrameRates);
    choice("sonic.dual_monitors", Display, sonic_text("Dual monitors","Dos monitores"),
           sonic_text("One DS screen per monitor. Reopen game.","Una LCD por monitor. Volve a abrir."),
           kSonicOffOnChoices);
    choice("sonic.dual_monitors_inverted", Display, sonic_text("Invert screens","Invertir pantallas"),
           sonic_text("Swap monitors when both screens are shown.","Cambia de monitor cada LCD visible."),
           kSonicOffOnChoices);
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
    choice("sonic.trick_button_jumps", Gameplay, sonic_text("Jump with Trick","Saltar con Truco"),
           sonic_text("Off uses only Jump to start a jump.","No: solo Saltar inicia el salto."),
           kSonicOffOnChoices);
    choice("sonic.battle_role", BattlePlay, sonic_text("Connection","Conexion"),
           sonic_text("Applies next launch.","Se aplica al reabrir."), kSonicBattleRoles);
    items.push_back({"sonic.battle_local_ip", g_sonic_sections[BattlePlay],
        sonic_text("Your IPv4","Tu IPv4"), sonic_text("Your LAN or VPN address.","Tu direccion LAN o VPN."),
        RECOMP_RUNTIME_UI_TEXT, 0, 15, 0, nullptr, 0, nullptr});
    items.push_back({"sonic.battle_peer_ip", g_sonic_sections[BattlePlay],
        sonic_text("Friend's IPv4","IPv4 de tu amigo"), sonic_text("Peer's LAN or VPN address.","Direccion LAN o VPN del otro jugador."),
        RECOMP_RUNTIME_UI_TEXT, 0, 15, 0, nullptr, 0, nullptr});
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
    if (std::strcmp(key,"sonic.internal_resolution")==0) return &g_sonic_pc.internal_resolution;
    if (std::strcmp(key,"sonic.filter")==0) return &g_sonic_pc.filter;
    if (std::strcmp(key,"sonic.border")==0) return &g_sonic_pc.border;
    if (std::strcmp(key,"sonic.menu_language")==0) return &g_sonic_pc.menu_language;
    if (std::strcmp(key,"sonic.game_language")==0) return &g_sonic_pc.game_language;
    if (std::strcmp(key,"sonic.aspect_mode")==0) return &g_sonic_pc.aspect_mode;
    if (std::strcmp(key,"sonic.frame_rate")==0) return &g_sonic_pc.frame_rate;
    if (std::strcmp(key,"sonic.dual_monitors")==0) return &g_sonic_pc.dual_monitors;
    if (std::strcmp(key,"sonic.dual_monitors_inverted")==0) return &g_sonic_pc.dual_monitors_inverted;
    if (std::strcmp(key,"sonic.trick_button_jumps")==0) return &g_sonic_pc.trick_button_jumps;
    if (std::strcmp(key,"sonic.battle_role")==0) return &g_sonic_pc.battle_role;
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
    if (preference == &g_sonic_pc.internal_resolution && value != *preference) {
        if (!state->apply_internal_resolution ||
            !state->apply_internal_resolution(value, previous.internal_resolution)) {
            if (state->ui) recomp_runtime_ui_set_status(state->ui,
                sonic_text("3D mode unavailable. Check OpenGL 4.3 support.",
                           "Modo 3D no disponible. Revisa OpenGL 4.3."));
            return 0;
        }
        *preference = value;
        return 1;
    }
    *preference = value;
    if (preference == &g_sonic_pc.trick_button_jumps ||
        preference == &g_sonic_pc.dual_monitors_inverted) return 1;
    if (preference == &g_sonic_pc.dual_monitors) {
        state->dual_monitors_changed = true;
        return 1;
    }
    if (preference == &g_sonic_pc.battle_role) {
        state->battle_changed = true;
        return 1;
    }
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

std::string* sonic_battle_address(const RecompRuntimeUiItem* item) {
    if (std::strcmp(item->key, "sonic.battle_local_ip") == 0) return &g_sonic_pc.battle_local_ip;
    if (std::strcmp(item->key, "sonic.battle_peer_ip") == 0) return &g_sonic_pc.battle_peer_ip;
    return nullptr;
}

int runtime_menu_get_text(void*, const RecompRuntimeUiItem* item, char* buffer, size_t size) {
    const auto* address = sonic_battle_address(item);
    if (!address) return 0;
    std::snprintf(buffer, size, "%s", address->c_str());
    return 1;
}

int runtime_menu_set_text(void* context, const RecompRuntimeUiItem* item, const char* value) {
    auto* state = static_cast<RuntimeMenuState*>(context);
    auto* address = sonic_battle_address(item);
    if (!state || !address) return 0;
    if (*value && !sonic_pc_battle_ipv4(value)) {
        recomp_runtime_ui_set_status(state->ui, sonic_text("Invalid IPv4 address","Direccion IPv4 invalida"));
        return 0;
    }
    *address = value;
    state->battle_changed = true;
    return 1;
}

void sonic_enter_address_text(RecompRuntimeUi* ui, std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return;
    const auto value = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
    if (value.size() <= 15 && value.find_first_not_of("0123456789.") == std::string_view::npos)
        recomp_runtime_ui_insert_text(ui, std::string(value).c_str());
}

void sonic_handle_address_key(RecompRuntimeUi* ui, SDL_Scancode code, SDL_Keycode key,
                              SDL_Keymod mod) {
    if (code == SDL_SCANCODE_ESCAPE)
        recomp_runtime_ui_edit_text(ui, RECOMP_RUNTIME_UI_TEXT_CANCEL);
    else if (code == SDL_SCANCODE_RETURN || code == SDL_SCANCODE_KP_ENTER)
        recomp_runtime_ui_edit_text(ui, RECOMP_RUNTIME_UI_TEXT_CONFIRM);
    else if (code == SDL_SCANCODE_BACKSPACE || code == SDL_SCANCODE_DELETE)
        recomp_runtime_ui_edit_text(ui, RECOMP_RUNTIME_UI_TEXT_BACKSPACE);
    else if ((mod & SDL_KMOD_CTRL) && code == SDL_SCANCODE_A)
        recomp_runtime_ui_edit_text(ui, RECOMP_RUNTIME_UI_TEXT_SELECT_ALL);
    else if ((mod & SDL_KMOD_CTRL) && code == SDL_SCANCODE_V) {
        char* clipboard = SDL_GetClipboardText();
        if (clipboard) {
            sonic_enter_address_text(ui, clipboard);
            SDL_free(clipboard);
        }
    } else if (!(mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI))) {
        if (key == SDLK_UNKNOWN) key = SDL_GetKeyFromScancode(code, mod, true);
        char digit = 0;
        if (key >= SDLK_0 && key <= SDLK_9) digit = static_cast<char>('0' + key - SDLK_0);
        else if (key >= SDLK_KP_1 && key <= SDLK_KP_9) digit = static_cast<char>('1' + key - SDLK_KP_1);
        else if (key == SDLK_KP_0) digit = '0';
        else if (key == SDLK_PERIOD || key == SDLK_KP_PERIOD || key == SDLK_KP_DECIMAL) digit = '.';
        if (digit) {
            const char value[] = {digit, 0};
            recomp_runtime_ui_insert_text(ui, value);
        }
    }
}

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
int runtime_menu_is_enabled(void*,const RecompRuntimeUiItem* item) {
    return std::strcmp(item->key, "sonic.dual_monitors_inverted") != 0 ||
        g_sonic_pc.dual_monitors;
}
void runtime_menu_save(void* context) {
    auto* state=static_cast<RuntimeMenuState*>(context);if(!state)return;
    const bool restart_notice = state->game_language_changed || state->dual_monitors_changed;
    const bool battle_notice = state->battle_changed;
    const bool refresh_notice = state->frame_rate_changed && g_sonic_pc.frame_rate == 2 &&
                                state->supports_high_refresh && !state->supports_high_refresh();
    state->game_language_changed = false;
    state->frame_rate_changed = false;
    state->dual_monitors_changed = false;
    state->battle_changed = false;
    const bool saved = sonic_pc_save(state->settings_path);
    if (!state->ui) return;
    if (!saved) recomp_runtime_ui_set_status(state->ui,sonic_text("Could not save settings","No se pudieron guardar las opciones"));
    else if (battle_notice) {
        const bool incomplete = g_sonic_pc.battle_role &&
            (g_sonic_pc.battle_local_ip.empty() || g_sonic_pc.battle_peer_ip.empty());
        recomp_runtime_ui_set_status(state->ui, incomplete
            ? sonic_text("Enter both IPv4 addresses","Completa ambas IPv4")
            : sonic_text("Saved. Reopen the game.","Guardado. Volve a abrir el juego."));
    }
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
