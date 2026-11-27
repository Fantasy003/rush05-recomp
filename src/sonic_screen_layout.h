// Geometry for the two native LCDs, shared by the renderer and touch input.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

struct SonicScreenRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct SonicScreenLayout {
    int width = 256;
    int height = 384;
    bool fill_output = false;
    std::array<SonicScreenRect,2> lcd{{{0, 0, 256, 192}, {0, 192, 256, 192}}};
};

// Each entry names the DS LCD drawn on that monitor. -1 clears the monitor.
inline std::array<int, 2> sonic_monitor_lcds(bool inverted, int focus, bool boss_hud = false) {
    if (focus >= 0) return {focus, boss_hud ? 1 - focus : -1};
    return inverted ? std::array{1, 0} : std::array{0, 1};
}

inline SonicScreenLayout sonic_screen_layout(
        int output_width, int output_height, int top_width, int bottom_width,
        int screen_height, int pad, int focus, bool fit, bool force_horizontal = false) {
    SonicScreenLayout layout;
    if (focus == 0 || focus == 1) {
        const int width = focus == 0 ? top_width : bottom_width;
        layout.width = width + 2 * pad;
        layout.height = screen_height + 2 * pad;
        layout.fill_output = fit;
        layout.lcd = {};
        layout.lcd[focus] = {pad, pad, width, screen_height};
        return layout;
    }

    const int width = std::max(top_width, bottom_width);
    const int stacked_width = width + 2 * pad;
    const int stacked_height = 2 * screen_height + 3 * pad;
    const int wide_width = top_width + bottom_width + 3 * pad;
    const int wide_height = screen_height + 2 * pad;
    // Compare uniform scales as rationals: use a row only if both LCDs
    // can be larger than they would be in a stack on this output.
    const bool side_by_side = force_horizontal ||
        (fit && output_width > 0 && output_height > 0 &&
         int64_t{output_width} * stacked_height > int64_t{output_height} * wide_width);
    if (side_by_side) {
        layout.width = wide_width;
        layout.height = wide_height;
        layout.lcd = {{{pad, pad, top_width, screen_height},
                       {top_width + 2 * pad, pad, bottom_width, screen_height}}};
    } else {
        layout.width = stacked_width;
        layout.height = stacked_height;
        layout.lcd = {{{pad + (width - top_width) / 2, pad, top_width, screen_height},
                       {pad + (width - bottom_width) / 2, screen_height + 2 * pad,
                        bottom_width, screen_height}}};
    }
    return layout;
}
