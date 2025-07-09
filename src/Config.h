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

// Helper: Pin name type (string)
using PinName = const char*;

// Pin mapping entry
struct PinMapEntry {
  PinName name;
  PinType type;
};

// ===========================
// 🎮 Logical Input Definitions
// ===========================

enum LogicalType : uint8_t {
  LOGICAL_BTN,
  LOGICAL_MATRIX
};

enum ButtonBehavior : uint8_t {
  NORMAL,
  MOMENTARY,
  ENC_A,  // Encoder channel A (clockwise)
  ENC_B   // Encoder channel B (counter-clockwise)
};

enum LogicalSource : uint8_t {
    SRC_PIN,
    SRC_MATRIX,
    SRC_SHIFTREG
};

struct LogicalShiftReg {
    uint8_t regIndex;    // 0-based shift register index (0..SHIFTREG_COUNT-1)
    uint8_t bitIndex;    // 0..7
    uint8_t joyButtonID;
    ButtonBehavior behavior;
};

struct LogicalInput {
  LogicalType type;
  union {
    struct { uint8_t pin; uint8_t joyButtonID; ButtonBehavior behavior; } btn;
    struct { uint8_t row; uint8_t col; uint8_t joyButtonID; ButtonBehavior behavior; } matrix;
    LogicalShiftReg shiftreg;
  } u;
  LogicalSource source;
};

// --- USER CONFIGURATION ---
// User must provide hardwarePinMap, hardwarePinMapCount, logicalInputs, logicalInputCount in UserConfig.h
#include "UserConfig.h"

constexpr uint8_t hardwarePinMapCount = sizeof(hardwarePinMap) / sizeof(hardwarePinMap[0]);
const uint8_t logicalInputCount = sizeof(logicalInputs) / sizeof(logicalInputs[0]);

// Lookup function: get PinType for a pin name (returns PIN_UNUSED if not found)
inline PinType getPinType(const PinName pin) {
  for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
    if (strcmp(hardwarePinMap[i].name, pin) == 0) {
      return hardwarePinMap[i].type;
    }
  }
  return PIN_UNUSED;
}