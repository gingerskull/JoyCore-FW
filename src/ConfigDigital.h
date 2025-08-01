// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// NOTE: This file is included by Config.h and depends on types defined there.
// The following types are used from Config.h:
// - PinMapEntry, PinType, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
// - LogicalInput, InputType, ButtonBehavior, LatchMode
// - INPUT_PIN, INPUT_MATRIX, INPUT_SHIFTREG, NORMAL, MOMENTARY, ENC_A, ENC_B, FOUR0, FOUR3

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
  {"4", BTN},
  {"5", BTN},
  {"6", BTN},
  {"7", BTN},
  {"8", BTN},
  {"9", BTN},
  {"10", BTN},
  {"11", BTN},
  {"12", BTN},
  {"13", BTN},
  //{"12", BTN_COL},
 // {"13", BTN_ROW},
  //{"14", BTN_ROW},
  //{"15", BTN_ROW},
  {"18", SHIFTREG_QH},   // Serial data out pin for 74HC165
 {"19", SHIFTREG_PL},  // Parallel load pin for 74HC165 
  {"20", SHIFTREG_CLK}    // Clock pin for 74HC165
  // ...add more as needed...
};

// ===========================
// USER EDITABLE SHIFT REGISTER CONFIG
// ===========================

// Example: 2 chained 74HC165
#define SHIFTREG_COUNT    2

// ===========================
// USER EDITABLE LOGICAL INPUTS
// ===========================

constexpr LogicalInput logicalInputs[] = {
  // Direct pin encoders
  //{ INPUT_PIN, { .pin = {6, 1, ENC_A, 0} }, FOUR3 },
  //{ INPUT_PIN, { .pin = {7, 2, ENC_B, 0} }, FOUR3 },

  // Multiple logical buttons on same physical pin - example of pin sharing
  { INPUT_PIN, { .pin = {4, 9, MOMENTARY, 0} } },    // Normal button on pin 4
  { INPUT_PIN, { .pin = {5, 10, MOMENTARY, 0} } },    // Reversed button on same pin 4
  { INPUT_PIN, { .pin = {6, 1, NORMAL, 0} } },    // Normal button on pin 4
  { INPUT_PIN, { .pin = {7, 2, NORMAL, 0} } },    // Reversed button on same pin 4
  { INPUT_PIN, { .pin = {8, 3, MOMENTARY, 0} } },    // Normal button on pin 4
  { INPUT_PIN, { .pin = {9, 4, MOMENTARY, 0} } },    // Reversed button on same pin 4
  { INPUT_PIN, { .pin = {10, 5, NORMAL, 0} } },    // Normal button on pin 4
  { INPUT_PIN, { .pin = {11, 6, NORMAL, 0} } },    // Reversed button on same pin 4
  { INPUT_PIN, { .pin = {12, 7, NORMAL, 0} } },    // Normal button on pin 4
  { INPUT_PIN, { .pin = {13, 8, NORMAL, 0} } },    // Reversed button on same pin 4

  // Matrix buttons (row, col, joyButtonID, behavior, reverse)
  //{ INPUT_MATRIX, { .matrix = {0, 0, 3, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {1, 0, 4, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {2, 0, 5, NORMAL, 0} } },    // Normal button at position 2,0
  //{ INPUT_MATRIX, { .matrix = {2, 0, 6, NORMAL, 1} } },    // Reversed button at same position 2,0

  // Encoders within matrix positions
  //{ INPUT_MATRIX, { .matrix = {2, 0, 3, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {3, 0, 4, NORMAL, 0} } },
 
  // Shift register inputs - multiple logical buttons on same physical bit
  
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 11, NORMAL, 0} } },    // Normal button on shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 12, NORMAL, 0} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 13, ENC_A, 0} }, FOUR0 },
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 14, ENC_B, 0} }, FOUR0 },    // Reversed button on same shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {0, 4, 15, NORMAL, 0} } },    // Normal button on shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {0, 5, 16, NORMAL, 0} } },
  { INPUT_SHIFTREG, { .shiftreg = {0, 6, 17, ENC_A, 0} }, FOUR0 },
  { INPUT_SHIFTREG, { .shiftreg = {0, 7, 18, ENC_B, 0} }, FOUR0 },    // Reversed button on same shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {1, 0, 19, NORMAL, 0} } },    // Normal button on shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {1, 1, 20, NORMAL, 0} } },
  { INPUT_SHIFTREG, { .shiftreg = {1, 2, 21, ENC_A, 0} }, FOUR0 },
  { INPUT_SHIFTREG, { .shiftreg = {1, 3, 22, ENC_B, 0} }, FOUR0 },    // Reversed button on same shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {1, 4, 23, NORMAL, 0} } },    // Normal button on shift reg 0, bit 2
  { INPUT_SHIFTREG, { .shiftreg = {1, 5, 24, NORMAL, 0} } },
  { INPUT_SHIFTREG, { .shiftreg = {1, 6, 25, ENC_A, 0} }, FOUR0 },
  { INPUT_SHIFTREG, { .shiftreg = {1, 7, 26, ENC_B, 0} }, FOUR0 },    // Reversed button on same shift reg 0, bit 2
  
  // ...add more as needed...
};