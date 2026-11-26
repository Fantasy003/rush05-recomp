// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

inline uint32_t sonic_jump_input_instruction(uint32_t pc, uint32_t instruction) {
    // USA gameplay overlay. These checks read the player's press edge before
    // dispatching its jump callback. A and B have bits 0 and 1, respectively.
    // Keep the original keypad state available to the trick and menu code.
    constexpr uint32_t kJumpOrTrick = 0xe2100003u; // ands r0, r0, #3
    constexpr uint32_t kJumpOnly = 0xe2100001u;    // ands r0, r0, #1
    if (instruction != kJumpOrTrick) return instruction;
    switch (pc) {
        case 0x023484b4u:
        case 0x0234b25cu:
        case 0x0234bb40u:
        case 0x0234c120u:
        case 0x0234e198u:
        case 0x0234e234u:
        case 0x0234f630u:
        case 0x0234f664u:
        case 0x02350858u:
        case 0x02350910u:
        case 0x02351254u:
        case 0x023512a4u:
        case 0x02351c58u:
        case 0x02351ce0u:
        case 0x02353c64u:
        case 0x02353d20u:
        case 0x023541e0u:
        case 0x023543c4u:
        case 0x02354b94u:
        case 0x02355490u:
            return kJumpOnly;
        default:
            return instruction;
    }
}
