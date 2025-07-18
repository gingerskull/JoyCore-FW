// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
  {"10", BTN},
  {"11", BTN},
  {"12", BTN_COL},
  {"13", BTN_ROW},
  {"14", BTN_ROW},
  {"15", BTN_ROW},
  {"18", SHIFTREG_QH},   // Serial data out pin for 74HC165
  {"19", SHIFTREG_PL},  // Parallel load pin for 74HC165 
  {"20", SHIFTREG_CLK}    // Clock pin for 74HC165
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
  { INPUT_PIN, { .pin = {10, 1, ENC_A} } },
  { INPUT_PIN, { .pin = {11, 2, ENC_B} } },

  // Regular buttons on pins 9,10 (joyButtons 20,21)
  //{ INPUT_PIN, { .pin = {4, 3, NORMAL} } },
  //{ INPUT_PIN, { .pin = {5, 4, NORMAL} } },

  // Matrix buttons (row, col, joyButtonID, behavior)
  { INPUT_MATRIX, { .matrix = {0, 0, 3, NORMAL} } },
  { INPUT_MATRIX, { .matrix = {1, 0, 4, NORMAL} } },
  { INPUT_MATRIX, { .matrix = {2, 0, 5, NORMAL} } },

  // Encoders within matrix positions
  //{ INPUT_MATRIX, { .matrix = {2, 0, 3, NORMAL} } },
  //{ INPUT_MATRIX, { .matrix = {3, 0, 4, NORMAL} } },
 
  // Shift register inputs
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 6, ENC_A} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 7, ENC_B} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 8, NORMAL} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 9, NORMAL} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 4, 10, NORMAL} } }
  // ...add more as needed...
};