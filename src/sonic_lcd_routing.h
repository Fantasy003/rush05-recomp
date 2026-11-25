// Host-only metadata that travels with the two native framebuffer buffers.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
struct SonicLcdRouting {
    std::array<std::array<unsigned,2>,2> lines{};
    void record(unsigned back, uint32_t dispcnt, bool main_on_top) {
        if(!(dispcnt&0x80u) && ((dispcnt>>16u)&3u)==1u)
            ++lines[back&1u][main_on_top?0:1];
    }
    void published(unsigned front) { lines[(front&1u)^1u]={}; }
    int screen(unsigned front) const {
        const auto& count=lines[front&1u];
        if(count[0]==0 && count[1]==0)return -1;
        return count[1]>count[0]?1:0;
    }
};
