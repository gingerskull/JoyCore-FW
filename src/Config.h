// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// ===========================
// 🛠️ Hardware Pin Definitions
// ===========================

// Pin types
enum PinType : uint8_t {
    PIN_UNUSED = 0,
    BTN,
    BTN_ROW,
    BTN_COL,
    SHIFTREG_PL,   // Parallel load (SH/LD)
    SHIFTREG_CLK,  // Clock
    SHIFTREG_QH    // Serial data out
};

// Helper: Pin name type (string) - renamed to avoid conflict with RP2040 framework PinName
using HardwarePinName = const char*;

// Pin mapping entry
struct PinMapEntry {
    HardwarePinName name;
    PinType type;
};

// ===========================
// 🎮 Logical Input Definitions
// ===========================

enum InputType : uint8_t {
    INPUT_PIN,
    INPUT_MATRIX,
    INPUT_SHIFTREG
};

enum ButtonBehavior : uint8_t {
    NORMAL,
    MOMENTARY,
    ENC_A,  // Encoder channel A (clockwise)
    ENC_B   // Encoder channel B (counter-clockwise)
};

struct LogicalInput {
    InputType type;
    union {
        struct { uint8_t pin; uint8_t joyButtonID; ButtonBehavior behavior; } pin;
        struct { uint8_t row; uint8_t col; uint8_t joyButtonID; ButtonBehavior behavior; } matrix;
        struct { uint8_t regIndex; uint8_t bitIndex; uint8_t joyButtonID; ButtonBehavior behavior; } shiftreg;
    } u;
};

// --- USER CONFIGURATION ---
// User must provide hardwarePinMap, hardwarePinMapCount, logicalInputs, logicalInputCount in UserConfig.h
#include "ConfigDigital.h"

constexpr uint8_t hardwarePinMapCount = sizeof(hardwarePinMap) / sizeof(hardwarePinMap[0]);
constexpr uint8_t logicalInputCount = sizeof(logicalInputs) / sizeof(logicalInputs[0]);

// Lookup function: get PinType for a pin name (returns PIN_UNUSED if not found)
inline PinType getPinType(HardwarePinName pin) {
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
        if (strcmp(hardwarePinMap[i].name, pin) == 0) {
            return hardwarePinMap[i].type;
        }
    }
    return PIN_UNUSED;
}