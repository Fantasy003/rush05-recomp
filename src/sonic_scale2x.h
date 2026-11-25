// Edge-aware 2x scaling for a DS framebuffer. Output is 2w by 2h.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>

inline void sonic_scale2x(const uint32_t* input, int width, int height,
                         uint32_t* output) {
    const int output_width = width * 2;
    for (int y = 0; y < height; ++y) {
        const auto* row = input + static_cast<size_t>(y) * width;
        const auto* above = y > 0 ? row - width : row;
        const auto* below = y + 1 < height ? row + width : row;
        auto* upper = output + static_cast<size_t>(2 * y) * output_width;
        auto* lower = upper + output_width;
        for (int x = 0; x < width; ++x) {
            const uint32_t center = row[x];
            const uint32_t north = above[x];
            const uint32_t south = below[x];
            const uint32_t west = row[std::max(0, x - 1)];
            const uint32_t east = row[std::min(width - 1, x + 1)];
            const bool corner = north != south && west != east;
            upper[2 * x] = corner && west == north ? west : center;
            upper[2 * x + 1] = corner && north == east ? east : center;
            lower[2 * x] = corner && west == south ? west : center;
            lower[2 * x + 1] = corner && south == east ? east : center;
        }
    }
}
