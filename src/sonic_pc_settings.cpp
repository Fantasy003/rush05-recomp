// SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "toml.hpp"
#include "frontend.h"
#include "sonic_pc_settings.h"

SonicPcPreferences sonic_pc_defaults() {
    SonicPcPreferences p;
    p.keys = {SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_BACKSPACE,
        SDL_SCANCODE_RETURN, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT,
        SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_W, SDL_SCANCODE_Q,
        SDL_SCANCODE_S, SDL_SCANCODE_A};
    p.buttons = {SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST,
        SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_START,
        SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
        SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
        SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_WEST};
    p.menu_keys = {SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT,
        SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_BACKSPACE};
    p.menu_buttons = {SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN,
        SDL_GAMEPAD_BUTTON_DPAD_LEFT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
        SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST};
    return p;
}
SonicPcPreferences g_sonic_pc = sonic_pc_defaults();

bool sonic_pc_battle_ipv4(const std::string& value, uint32_t* address) {
    if (value.size() > 15) return false;
    uint32_t parsed = 0;
    if (!nds_parse_ipv4(value, &parsed) || (parsed >> 24) == 0 || (parsed >> 24) >= 224)
        return false;
    if (address) *address = parsed;
    return true;
}

static bool valid_key(int value) {
    return value > SDL_SCANCODE_UNKNOWN && value < SDL_SCANCODE_COUNT &&
        value != SDL_SCANCODE_ESCAPE && value != SDL_SCANCODE_TAB &&
        !(value >= SDL_SCANCODE_F1 && value <= SDL_SCANCODE_F12);
}
static bool valid_button(int value) {
    return value >= 0 && value < SDL_GAMEPAD_BUTTON_COUNT &&
        value != SDL_GAMEPAD_BUTTON_GUIDE;
}
template<std::size_t Count, typename Check>
static bool unique_bindings(const toml::array* a, std::array<int,Count>& target, Check check) {
    if (!a || a->size() != target.size()) return false;
    std::array<int,Count> parsed{};
    for (size_t i=0;i<parsed.size();++i) {
        auto number = (*a)[i].value<int64_t>();
        if (!number || *number < 0 || *number > 4096 || !check(static_cast<int>(*number))) return false;
        parsed[i] = static_cast<int>(*number);
        for (size_t j=0;j<i;++j) if (parsed[j] == parsed[i]) return false;
    }
    target = parsed;
    return true;
}
void sonic_pc_load(const std::string& path) {
    g_sonic_pc = sonic_pc_defaults();
    if (path.empty() || !std::filesystem::exists(path)) return;
    try {
        const auto t = toml::parse_file(path);
        if (t["version"].value_or(0) != 1) return;
        auto bounded = [&](const char* key,int maximum,int& value) {
            if (const auto v=t[key].value<int64_t>(); v && *v>=0 && *v<=maximum) value=static_cast<int>(*v);
        };
        bounded("screen_mode",1,g_sonic_pc.screen_mode);
        bounded("window_mode",2,g_sonic_pc.window_mode);
        bounded("resolution",8,g_sonic_pc.resolution);
    bounded("internal_resolution", static_cast<int>(kSonicRasterHeights.size()) - 1,
            g_sonic_pc.internal_resolution);
        bounded("filter",2,g_sonic_pc.filter);
        bounded("border",2,g_sonic_pc.border);
        bounded("menu_language",1,g_sonic_pc.menu_language);
        bounded("game_language",1,g_sonic_pc.game_language);
        bounded("aspect_mode",1,g_sonic_pc.aspect_mode);
        bounded("frame_rate",2,g_sonic_pc.frame_rate);
        bounded("dual_monitors",1,g_sonic_pc.dual_monitors);
        bounded("dual_monitors_inverted",1,g_sonic_pc.dual_monitors_inverted);
        bounded("trick_button_jumps",1,g_sonic_pc.trick_button_jumps);
        bounded("battle_role",2,g_sonic_pc.battle_role);
        auto address = [&](const char* key) {
            auto value = t[key].value_or(std::string{});
            return sonic_pc_battle_ipv4(value) ? value : std::string{};
        };
        g_sonic_pc.battle_local_ip = address("battle_local_ip");
        g_sonic_pc.battle_peer_ip = address("battle_peer_ip");
        unique_bindings(t["keys"].as_array(),g_sonic_pc.keys,valid_key);
        unique_bindings(t["buttons"].as_array(),g_sonic_pc.buttons,valid_button);
        unique_bindings(t["menu_keys"].as_array(),g_sonic_pc.menu_keys,valid_key);
        unique_bindings(t["menu_buttons"].as_array(),g_sonic_pc.menu_buttons,valid_button);
    } catch (const std::exception& e) {
        std::fprintf(stderr,"[sonic-pc] Cannot load settings: %s\n",e.what());
    }
}
bool sonic_pc_save(const std::string& path) {
    if (path.empty()) return false;
    try {
        std::filesystem::path destination(path), temporary=destination;
        temporary += ".tmp";
        if (!destination.parent_path().empty()) std::filesystem::create_directories(destination.parent_path());
        {
            std::ofstream out(temporary,std::ios::trunc);
            if (!out) return false;
            out << "version = 1\nscreen_mode = " << g_sonic_pc.screen_mode
                << "\nwindow_mode = " << g_sonic_pc.window_mode
                << "\nresolution = " << g_sonic_pc.resolution
    << "\ninternal_resolution = " << g_sonic_pc.internal_resolution
                << "\nfilter = " << g_sonic_pc.filter
                << "\nborder = " << g_sonic_pc.border
                << "\nmenu_language = " << g_sonic_pc.menu_language
                << "\ngame_language = " << g_sonic_pc.game_language
                << "\naspect_mode = " << g_sonic_pc.aspect_mode
                << "\nframe_rate = " << g_sonic_pc.frame_rate
                << "\ndual_monitors = " << g_sonic_pc.dual_monitors
                << "\ndual_monitors_inverted = " << g_sonic_pc.dual_monitors_inverted
                << "\ntrick_button_jumps = " << g_sonic_pc.trick_button_jumps
                << "\nbattle_role = " << g_sonic_pc.battle_role
                << "\nbattle_local_ip = \"" << g_sonic_pc.battle_local_ip << '"'
                << "\nbattle_peer_ip = \"" << g_sonic_pc.battle_peer_ip << "\"\n";
            auto write_array = [&](const char* name,const auto& values) {
                out << name << " = [";
                for (size_t i=0;i<values.size();++i) { if(i)out<<", ";out<<values[i]; }
                out << "]\n";
            };
            write_array("keys",g_sonic_pc.keys);
            write_array("buttons",g_sonic_pc.buttons);
            write_array("menu_keys",g_sonic_pc.menu_keys);
            write_array("menu_buttons",g_sonic_pc.menu_buttons);
            out.flush();
            if (!out) return false;
        }
        return MoveFileExW(temporary.c_str(),destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr,"[sonic-pc] Cannot save settings: %s\n",e.what());
        return false;
    }
}
template<std::size_t Count>
static void swap_binding(std::array<int,Count>& values, unsigned action, int value) {
    const int old=values[action];
    for (unsigned i=0;i<values.size();++i) if (i!=action && values[i]==value) values[i]=old;
    values[action]=value;
}
bool sonic_pc_bind_key(unsigned action,int scancode) {
    if(action>=12 || !valid_key(scancode)) return false;
    swap_binding(g_sonic_pc.keys,action,scancode);return true;
}
bool sonic_pc_bind_button(unsigned action,int button) {
    if(action>=12 || !valid_button(button)) return false;
    swap_binding(g_sonic_pc.buttons,action,button);return true;
}
bool sonic_pc_bind_menu_key(unsigned action, int scancode) {
    if (action >= kSonicMenuActionCount || !valid_key(scancode)) return false;
    swap_binding(g_sonic_pc.menu_keys, action, scancode);
    return true;
}
bool sonic_pc_bind_menu_button(unsigned action, int button) {
    if (action >= kSonicMenuActionCount || !valid_button(button)) return false;
    swap_binding(g_sonic_pc.menu_buttons, action, button);
    return true;
}
SonicMenuAction sonic_pc_menu_key_action(int scancode) {
    const auto& keys = g_sonic_pc.menu_keys;
    const auto match = std::find(keys.begin(), keys.end(), scancode);
    if (match != keys.end()) return static_cast<SonicMenuAction>(match - keys.begin());
    if (scancode == SDL_SCANCODE_KP_ENTER &&
        keys[static_cast<std::size_t>(SonicMenuAction::Confirm)] == SDL_SCANCODE_RETURN)
        return SonicMenuAction::Confirm;
    return SonicMenuAction::Count;
}
SonicMenuAction sonic_pc_menu_button_action(int button) {
    const auto& buttons = g_sonic_pc.menu_buttons;
    const auto match = std::find(buttons.begin(), buttons.end(), button);
    if (match != buttons.end()) return static_cast<SonicMenuAction>(match - buttons.begin());
    if (button == SDL_GAMEPAD_BUTTON_BACK &&
        buttons[static_cast<std::size_t>(SonicMenuAction::Back)] == SDL_GAMEPAD_BUTTON_EAST)
        return SonicMenuAction::Back;
    return SonicMenuAction::Count;
}
unsigned sonic_pc_key_mask(int scancode) {
    for(unsigned i=0;i<12;++i)if(g_sonic_pc.keys[i]==scancode)return 1u<<i;
    return 0;
}
unsigned sonic_pc_button_mask(int button) {
    for(unsigned i=0;i<12;++i)if(g_sonic_pc.buttons[i]==button)return 1u<<i;
    return 0;
}
int sonic_pc_focus_screen(bool enabled,bool player,int screen) {
    return enabled && player && (screen==0 || screen==1) ? screen : -1;
}

std::string sonic_pc_settings_path(const std::string& runtime_settings_path) {
    if (runtime_settings_path.empty()) return {};
    return (std::filesystem::path(runtime_settings_path).parent_path() /
            "sonic-settings.toml").string();
}
