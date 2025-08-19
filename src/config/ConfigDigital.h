#pragma once

// Configuration entry point for digital inputs used by ButtonInput, EncoderInput, and MatrixInput.
// This file is included by Config.h and uses the following types from it:
// - PinMapEntry, PinType, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
// - LogicalInput, InputType, ButtonBehavior, LatchMode
// - INPUT_PIN, INPUT_MATRIX, INPUT_SHIFTREG, NORMAL, MOMENTARY, ENC_A, ENC_B, FOUR0, FOUR3
//
// Electrical semantics used by the runtime:
// - Direct pins are configured with INPUT_PULLUP; a physical press reads LOW.
// - 74HC165 shift-register bits are active-low; the code inverts so a pressed bit is treated as true.
// - Matrix scanning is available but disabled until BTN_ROW/BTN_COL pins are present here and INPUT_MATRIX entries exist below.
// - Momentary buttons emit a brief press pulse on rising edge (handled in ButtonInput/MatrixInput).
// - Encoders are defined as adjacent ENC_A then ENC_B logical entries; latch mode is taken from the ENC_* entries (FOUR0/FOUR3/etc).

// ===========================
// USER EDITABLE PIN MAPPING
// ===========================

static const PinMapEntry hardwarePinMap[] = {
  // Only specify used pins; unused pins do not need to be listed.
  // Available pin types: PIN_UNUSED, BTN, BTN_ROW, BTN_COL, SHIFTREG_PL, SHIFTREG_CLK, SHIFTREG_QH
  // Direct input pins use internal pull-ups (LOW = pressed). Shift-register bits are also active-low.
  
  // Direct button inputs (pins 4-13 in current config)
  {"4", BTN},   // Button 9 (MOMENTARY)
  {"5", BTN},   // Button 10 (MOMENTARY)
  {"6", BTN},   // Button 1 (NORMAL)
  {"7", BTN},   // Button 2 (NORMAL)
  {"8", BTN},   // Button 3 (MOMENTARY)
  {"9", BTN},   // Button 4 (MOMENTARY)
  {"10", BTN},  // Button 5 (NORMAL)
  {"11", BTN},  // Button 6 (NORMAL)
  {"12", BTN},  // Button 7 (NORMAL)
  {"13", BTN},  // Button 8 (NORMAL)
  
  // Matrix configuration (available but disabled until enabled here and in logicalInputs)
  //{"12", BTN_COL},
  //{"13", BTN_ROW},
  //{"14", BTN_ROW},
  //{"15", BTN_ROW},
  
  // 74HC165 shift register control pins
  {"18", SHIFTREG_QH},   // Serial data out pin for 74HC165
  {"19", SHIFTREG_PL},   // Parallel load pin for 74HC165 
  {"20", SHIFTREG_CLK}   // Clock pin for 74HC165
  // ...add more as needed...
};

// ===========================
// USER EDITABLE SHIFT REGISTER CONFIG
// ===========================

 // 74HC165 configuration: number of chained devices (8 inputs each). Active-low inputs.
 // Current configuration uses 2 devices (16 inputs). Increase this if you chain more 74HC165s
 // and add corresponding INPUT_SHIFTREG entries in logicalInputs. Encoders must be defined as
 // consecutive bits (ENC_A followed by ENC_B) and can specify FOUR0/FOUR3 latch modes per pair.
 #define SHIFTREG_COUNT    2

// ===========================
// USER EDITABLE LOGICAL INPUTS
// ===========================

constexpr LogicalInput logicalInputs[] = {
  // Encoders on direct pins (example disabled). Encoders must be adjacent: ENC_A then ENC_B.
  // Latch mode can be specified per pair (default FOUR3 if omitted).
  //{ INPUT_PIN, { .pin = {6, 1, ENC_A, 0} }, FOUR3 },
  //{ INPUT_PIN, { .pin = {7, 2, ENC_B, 0} }, FOUR3 },

  // Direct pin buttons (LOW = pressed due to INPUT_PULLUP). Joystick IDs are 1-based.
  //{ INPUT_PIN, { .pin = {6, 1, ENC_A, 0} }, FOUR3 },
  //{ INPUT_PIN, { .pin = {7, 2, ENC_B, 0} }, FOUR3 },

  // Direct pin buttons (pins 4-13 mapped to joystick buttons 1-10)
  { INPUT_PIN, { .pin = {4, 9, MOMENTARY, 0} } },    // Pin 4 -> Button 9 (MOMENTARY)
  { INPUT_PIN, { .pin = {5, 10, MOMENTARY, 0} } },   // Pin 5 -> Button 10 (MOMENTARY)
  { INPUT_PIN, { .pin = {6, 1, NORMAL, 0} } },       // Pin 6 -> Button 1 (NORMAL)
  { INPUT_PIN, { .pin = {7, 2, NORMAL, 0} } },       // Pin 7 -> Button 2 (NORMAL)
  { INPUT_PIN, { .pin = {8, 3, MOMENTARY, 0} } },    // Pin 8 -> Button 3 (MOMENTARY)
  { INPUT_PIN, { .pin = {9, 4, MOMENTARY, 0} } },    // Pin 9 -> Button 4 (MOMENTARY)
  { INPUT_PIN, { .pin = {10, 5, NORMAL, 0} } },      // Pin 10 -> Button 5 (NORMAL)
  { INPUT_PIN, { .pin = {11, 6, NORMAL, 0} } },      // Pin 11 -> Button 6 (NORMAL)
  { INPUT_PIN, { .pin = {12, 7, NORMAL, 0} } },      // Pin 12 -> Button 7 (NORMAL)
  { INPUT_PIN, { .pin = {13, 8, NORMAL, 0} } },      // Pin 13 -> Button 8 (NORMAL)

  // Matrix buttons (available but disabled). To enable:
  // 1) Add BTN_ROW/BTN_COL pins to hardwarePinMap above.
  // 2) Add INPUT_MATRIX entries here for each row/col position you need.
  //{ INPUT_MATRIX, { .matrix = {0, 0, 3, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {1, 0, 4, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {2, 0, 5, NORMAL, 0} } },    // Normal button at position 2,0
  //{ INPUT_MATRIX, { .matrix = {2, 0, 6, NORMAL, 1} } },    // Reversed button at same position 2,0

  // Encoders within matrix positions (available but disabled). Encoders still require adjacent ENC_A then ENC_B entries.
  //{ INPUT_MATRIX, { .matrix = {2, 0, 3, NORMAL, 0} } },
  //{ INPUT_MATRIX, { .matrix = {3, 0, 4, NORMAL, 0} } },
 
  // Shift-register inputs (active-low) — Register 0 (bits 0-7)
  { INPUT_SHIFTREG, { .shiftreg = {0, 0, 11, NORMAL, 0} } },     // Reg 0, bit 0 -> Button 11
  { INPUT_SHIFTREG, { .shiftreg = {0, 1, 12, NORMAL, 0} } },     // Reg 0, bit 1 -> Button 12
  { INPUT_SHIFTREG, { .shiftreg = {0, 2, 13, ENC_A, 0} }, FOUR0 }, // Reg 0, bit 2 -> Encoder A (Button 13)
  { INPUT_SHIFTREG, { .shiftreg = {0, 3, 14, ENC_B, 0} }, FOUR0 }, // Reg 0, bit 3 -> Encoder B (Button 14)
  { INPUT_SHIFTREG, { .shiftreg = {0, 4, 15, NORMAL, 0} } },     // Reg 0, bit 4 -> Button 15
  { INPUT_SHIFTREG, { .shiftreg = {0, 5, 16, NORMAL, 0} } },     // Reg 0, bit 5 -> Button 16
  { INPUT_SHIFTREG, { .shiftreg = {0, 6, 17, ENC_A, 0} }, FOUR0 }, // Reg 0, bit 6 -> Encoder A (Button 17)
  { INPUT_SHIFTREG, { .shiftreg = {0, 7, 18, ENC_B, 0} }, FOUR0 }, // Reg 0, bit 7 -> Encoder B (Button 18)
  
  // Shift-register inputs (active-low) — Register 1 (bits 0-7)
  { INPUT_SHIFTREG, { .shiftreg = {1, 0, 19, NORMAL, 0} } },     // Reg 1, bit 0 -> Button 19
  { INPUT_SHIFTREG, { .shiftreg = {1, 1, 20, NORMAL, 0} } },     // Reg 1, bit 1 -> Button 20
  { INPUT_SHIFTREG, { .shiftreg = {1, 2, 21, ENC_A, 0} }, FOUR0 }, // Reg 1, bit 2 -> Encoder A (Button 21)
  { INPUT_SHIFTREG, { .shiftreg = {1, 3, 22, ENC_B, 0} }, FOUR0 }, // Reg 1, bit 3 -> Encoder B (Button 22)
  { INPUT_SHIFTREG, { .shiftreg = {1, 4, 23, NORMAL, 0} } },     // Reg 1, bit 4 -> Button 23
  { INPUT_SHIFTREG, { .shiftreg = {1, 5, 24, NORMAL, 0} } },     // Reg 1, bit 5 -> Button 24
  { INPUT_SHIFTREG, { .shiftreg = {1, 6, 25, ENC_A, 0} }, FOUR0 }, // Reg 1, bit 6 -> Encoder A (Button 25)
  { INPUT_SHIFTREG, { .shiftreg = {1, 7, 26, ENC_B, 0} }, FOUR0 }, // Reg 1, bit 7 -> Encoder B (Button 26)
  
  // ...add more as needed...
};

// ===========================
// USB DESCRIPTOR CONFIGURATION
// ===========================

// USB descriptor defaults used when generating a new config.bin after firmware version change
// or when no valid stored configuration exists.
static const struct {
    uint16_t vendorID;
    uint16_t productID;
    const char* manufacturer;
    const char* product;
} staticUSBDescriptor = {
    .vendorID = 0x2E8A,          // Raspberry Pi Foundation VID
    .productID = 0xA02F,         // Custom PID for JoyCore
    .manufacturer = "Gingerskull",
    .product = "Joycore Controller"
};
