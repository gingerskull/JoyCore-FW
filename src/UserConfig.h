#pragma once

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL
  {"3", BTN_ROW},
  {"4", BTN_ROW},
  {"5", BTN_ROW},
  {"6", BTN_ROW},
  {"7", BTN_COL},
  {"8", BTN_COL}
  // ...add more as needed...
};

// ===========================
// USER EDITABLE LOGICAL INPUTS
// ===========================

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
  { LOGICAL_MATRIX,  { .matrix = {3, 0, 27, ENC_B} } },   // Row 3 (pin 6) -> Joy 27 (CCW)
  { LOGICAL_MATRIX,  { .matrix = {0, 1, 28, ENC_A} } },  // Row 2 (pin 5) -> Joy 26 (CW)
  { LOGICAL_MATRIX,  { .matrix = {1, 1, 29, ENC_B} } },   // Row 3 (pin 6) -> Joy 27 (CCW)
  { LOGICAL_MATRIX,  { .matrix = {2, 1, 30, NORMAL} } },  // Row 2 (pin 5) -> Joy 26 (CW)
  { LOGICAL_MATRIX,  { .matrix = {3, 1, 31, NORMAL} } }   // Row 3 (pin 6) -> Joy 27 (CCW)
  
};