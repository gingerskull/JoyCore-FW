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
  ENC_A,
  ENC_B
};

// Indexed by pin number, value = PinType
constexpr PinType hardwarePins[] = {
  /* 0 */ PIN_UNUSED,
  /* 1 */ PIN_UNUSED,
  /* 2 */ BTN,
  /* 3 */ BTN,
  /* 4 */ ENC_A,
  /* 5 */ ENC_B,
  /* 6 */ ENC_A,
  /* 7 */ ENC_B,
  /* 8 */ BTN,
  /* 9 */ BTN_ROW,
  /*10 */ BTN_ROW,
  /*11 */ BTN_COL,
  /*12 */ BTN_COL,
  /*13 */ BTN_COL
  // ...add more pins as needed
};

// ===========================
// 🎮 Logical Input Definitions
// ===========================

enum LogicalType : uint8_t {
  LOGICAL_BTN,
  LOGICAL_ENCODER,
  LOGICAL_MATRIX
};

enum ButtonBehavior : uint8_t {
  NORMAL,
  MOMENTARY
};

struct LogicalInput {
  LogicalType type;
  union {
    struct { uint8_t pin; uint8_t joyButtonID; ButtonBehavior behavior; } btn;
    struct { uint8_t pinA; uint8_t pinB; uint8_t joyCw; uint8_t joyCcw; } encoder;
    struct { uint8_t row; uint8_t col; uint8_t joyButtonID; ButtonBehavior behavior; } matrix;
  } u;
};

// Only logical mapping, no pin config here
constexpr LogicalInput logicalInputs[] = {
  // Buttons
  { LOGICAL_BTN,     { .btn = {2, 0, NORMAL} } },
  { LOGICAL_BTN,     { .btn = {3, 1, NORMAL} } },
  { LOGICAL_BTN,     { .btn = {8, 2, MOMENTARY} } },
  // Encoders
  { LOGICAL_ENCODER, { .encoder = {4, 5, 20, 21} } },
  { LOGICAL_ENCODER, { .encoder = {6, 7, 22, 23} } },
  // Matrix buttons (row, col, joyButtonID, behavior)
  { LOGICAL_MATRIX,  { .matrix = {0, 0, 24, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {0, 1, 25, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {0, 2, 26, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {1, 0, 27, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {1, 1, 28, NORMAL} } },
  { LOGICAL_MATRIX,  { .matrix = {1, 2, 29, NORMAL} } }
};
constexpr uint8_t logicalInputCount = sizeof(logicalInputs) / sizeof(logicalInputs[0]);