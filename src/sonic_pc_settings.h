// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

enum class SonicMenuAction { Up, Down, Left, Right, Confirm, Back, Count };
inline constexpr std::size_t kSonicMenuActionCount =
    static_cast<std::size_t>(SonicMenuAction::Count);

struct SonicPcPreferences {
    int screen_mode = 0;   // 0 NDS, 1 Concentrado
    int window_mode = 0;   // 0 windowed, 1 exclusive, 2 borderless
    int resolution = 0;
    int filter = 0;
    int border = 0;        // 0 original, 1 black, 2 graphite frame
    int menu_language = 1; // 0 English, 1 Spanish
    int game_language = 0;
    int aspect_mode = 0;   // 0 stacked, 1 choose layout to fit the display
    int frame_rate = 1;    // 0 = 30, 1 = native (~60), 2 = blended up to ~120
    int trick_button_jumps = 1;
    int battle_role = 0;   // 0 offline, 1 host, 2 join
    std::string battle_local_ip;
    std::string battle_peer_ip;
    std::array<int,12> keys{};
    std::array<int,12> buttons{};
    std::array<int,kSonicMenuActionCount> menu_keys{};
    std::array<int,kSonicMenuActionCount> menu_buttons{};
};
extern SonicPcPreferences g_sonic_pc;
SonicPcPreferences sonic_pc_defaults();
void sonic_pc_load(const std::string& path);
bool sonic_pc_save(const std::string& path);
bool sonic_pc_battle_ipv4(const std::string& value, uint32_t* address = nullptr);
bool sonic_pc_bind_key(unsigned action, int scancode);
bool sonic_pc_bind_button(unsigned action, int button);
bool sonic_pc_bind_menu_key(unsigned action, int scancode);
bool sonic_pc_bind_menu_button(unsigned action, int button);
unsigned sonic_pc_key_mask(int scancode);
unsigned sonic_pc_button_mask(int button);
SonicMenuAction sonic_pc_menu_key_action(int scancode);
SonicMenuAction sonic_pc_menu_button_action(int button);
// The published frame's main engine is only followed during a live player task.
// Menus and unknown scenes retain both LCDs, including their touch controls.
int sonic_pc_focus_screen(bool enabled, bool player_active, int published_main_screen);
std::string sonic_pc_settings_path(const std::string& runtime_settings_path);
