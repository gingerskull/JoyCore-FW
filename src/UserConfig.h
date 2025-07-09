// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
  {"2", BTN_ROW},
  {"3", BTN_ROW},
  {"4", BTN_ROW},
  {"5", BTN_ROW},
  {"6", BTN_COL},
  {"16", SHIFTREG_QH},   // Serial data out pin for 74HC165
  {"14", SHIFTREG_PL},  // Parallel load pin for 74HC165 
  {"15", SHIFTREG_CLK}    // Clock pin for 74HC165
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
  // Buttons
  //{ LOGICAL_BTN,     { .btn = {2, 0, NORMAL} }, SRC_PIN },
  //{ LOGICAL_BTN,     { .btn = {3, 1, NORMAL} }, SRC_PIN },

  // Encoders as adjacent button pairs (pinA=4, pinB=5 -> joyButtons 20,21)
  //{ LOGICAL_BTN,     { .btn = {4, 20, ENC_A} }, SRC_PIN },
  //{ LOGICAL_BTN,     { .btn = {5, 21, ENC_B} }, SRC_PIN },

  // Matrix buttons (row, col, joyButtonID, behavior)
  { LOGICAL_MATRIX,  { .matrix = {0, 0, 24, NORMAL} }, SRC_MATRIX },
  { LOGICAL_MATRIX,  { .matrix = {1, 0, 25, NORMAL} }, SRC_MATRIX },

  // Encoders within matrix positions - encoder system will claim the pins
  { LOGICAL_MATRIX,  { .matrix = {2, 0, 26, ENC_A} }, SRC_MATRIX },
  { LOGICAL_MATRIX,  { .matrix = {3, 0, 27, ENC_B} }, SRC_MATRIX },
 
  // Shift register buttons format 
  // { LOGICAL_BTN, { .shiftreg = {shiftregIndex, bitIndex, joyButtonID, behavior} }, SRC_SHIFTREG }
  { LOGICAL_BTN, { .shiftreg = {0, 0, 5, NORMAL} }, SRC_SHIFTREG },
  { LOGICAL_BTN, { .shiftreg = {0, 1, 6, ENC_A} }, SRC_SHIFTREG },
  { LOGICAL_BTN, { .shiftreg = {0, 2, 7, ENC_B} }, SRC_SHIFTREG }
  // ...add more as needed...
};