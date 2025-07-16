// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================
// For RP2040: Use GPIO pin numbers (0-28)
// Example: "7" refers to GPIO7, "15" refers to GPIO15
// Analog-capable pins are GPIO26 (A0), GPIO27 (A1), GPIO28 (A2)

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
  {"7", BTN},
  {"8", BTN},
  {"9", BTN},
  {"10", BTN},
  //{"11", BTN},
  {"15", SHIFTREG_QH},   // Serial data out pin for 74HC165
  {"16", SHIFTREG_PL},  // Parallel load pin for 74HC165 
  {"17", SHIFTREG_CLK}    // Clock pin for 74HC165
  // ...add more as needed...
};

// ===========================
// USER EDITABLE SHIFT REGISTER CONFIG
// ===========================

// Example: 2 chained 74HC165
#define SHIFTREG_COUNT    1

// ===========================
// USER EDITABLE LOGICAL INPUTS
// ===========================

constexpr LogicalInput logicalInputs[] = {
  // Direct pin buttons
  { INPUT_PIN, { .pin = {7, 1, ENC_A} } },
  { INPUT_PIN, { .pin = {8, 2, ENC_B} } },

  // Regular buttons on pins 9,10 (joyButtons 20,21)
  { INPUT_PIN, { .pin = {9, 20, NORMAL} } },
  { INPUT_PIN, { .pin = {10, 21, NORMAL} } },

  // Matrix buttons (row, col, joyButtonID, behavior)
  //{ INPUT_MATRIX, { .matrix = {0, 0, 1, ENC_A} } },
  //{ INPUT_MATRIX, { .matrix = {1, 0, 2, ENC_B} } },

  // Encoders within matrix positions
  //{ INPUT_MATRIX, { .matrix = {2, 0, 3, NORMAL} } },
  //{ INPUT_MATRIX, { .matrix = {3, 0, 4, NORMAL} } },
 
  // Shift register inputs
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 10, NORMAL} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 5, ENC_A} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 6, ENC_B} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 7, NORMAL} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 4, 8, NORMAL} } }
  // ...add more as needed...
};