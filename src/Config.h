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
  BTN_COL
};

// Indexed by pin number, value = PinType
constexpr PinType hardwarePins[] = {
  /* 0 */ PIN_UNUSED,
  /* 1 */ PIN_UNUSED,
  /* 2 */ PIN_UNUSED,  
  /* 3 */ BTN_ROW,
  /* 4 */ BTN_ROW,  // Can be used for encoder or matrix
  /* 5 */ BTN_ROW,  // Can be used for encoder or matrix  
  /* 6 */ BTN_ROW,
  /* 7 */ BTN_COL
  // ...add more pins as needed
};

constexpr uint8_t hardwarePinsCount = sizeof(hardwarePins) / sizeof(hardwarePins[0]);

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

struct LogicalInput {
  LogicalType type;
  union {
    struct { uint8_t pin; uint8_t joyButtonID; ButtonBehavior behavior; } btn;
    struct { uint8_t row; uint8_t col; uint8_t joyButtonID; ButtonBehavior behavior; } matrix;
  } u;
};

// Only logical mapping, no pin config here
constexpr LogicalInput logicalInputs[] = {
  // Buttons
  //{ LOGICAL_BTN,     { .btn = {2, 0, NORMAL} } },
  //{ LOGICAL_BTN,     { .btn = {3, 1, NORMAL} } },
  
  // Encoders as adjacent button pairs (pinA=4, pinB=5 -> joyButtons 20,21)
  //{ LOGICAL_BTN,     { .btn = {4, 20, ENC_A} } },
  //{ LOGICAL_BTN,     { .btn = {5, 21, ENC_B} } },
  
  // Matrix buttons (row, col, joyButtonID, behavior)
  { LOGICAL_MATRIX,  { .matrix = {0, 0, 24, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {1, 0, 25, NORMAL} } },
  
  // Encoders within matrix positions - encoder system will claim the pins
  { LOGICAL_MATRIX,  { .matrix = {2, 0, 26, ENC_A} } },  // Row 2 (pin 5) -> Joy 26 (CW)
  { LOGICAL_MATRIX,  { .matrix = {3, 0, 27, ENC_B} } }   // Row 3 (pin 6) -> Joy 27 (CCW)
  
};
constexpr uint8_t logicalInputCount = sizeof(logicalInputs) / sizeof(logicalInputs[0]);