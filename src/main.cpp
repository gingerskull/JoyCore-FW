// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Custom Button Box Controller for Flight Simulators and Gaming
 * 
 * This Arduino-based project creates a 32-button USB game controller that handles
 * multiple input types for HOTAS (Hands On Throttle And Stick) setups.
 * 
 * Features:
 * - Matrix Buttons: Efficient button scanning using row/column pin layout
 * - Rotary Encoders: Converts encoder rotation to momentary button presses
 * - Direct Buttons: Individual buttons wired directly to pins
 * - Mixed Input Types: Encoders can use matrix or direct pin connections
 * - Shift Register Support: 74HC165 chips for additional inputs
 * 
 * The system automatically configures inputs based on logical definitions in Config.h
 * and appears as a standard USB gamepad to the host system.
 */

#include <Arduino.h>
#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"
#include "MatrixInput.h"
#include "ShiftRegister165.h"

// Initialize USB joystick with 32 buttons and no analog axes
// This creates a standard gamepad that Windows will recognize
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   32, 0, false, false, false,
                   false, false, false,
                   false, false, false);

// External references to shift register components (defined in ButtonInput.cpp)
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
  // Start the USB joystick interface
  Joystick.begin();
  
  // Initialize all input subsystems based on user configuration in UserConfig.h
  // Each init function parses the logicalInputs array and sets up the appropriate hardware
  
  // Initialize direct pin buttons and shift register buttons
  // Excludes encoder pins which are handled separately
  initButtonsFromLogical(logicalInputs, logicalInputCount);
  
  // Initialize rotary encoders (matrix, direct pin, and shift register)
  // Uses RotaryEncoder library for matrix/direct pins, custom decoder for shift register
  initEncodersFromLogical(logicalInputs, logicalInputCount);
  
  // Initialize button matrix scanning
  // Sets up row/col pins and configures keypad library
  initMatrixFromLogical(logicalInputs, logicalInputCount);
}

void loop() {
  // Read shift register buffer ONCE per loop cycle for consistent data
  // This ensures both buttons and encoders see the same pin states
  if (shiftReg && shiftRegBuffer) {
    shiftReg->read(shiftRegBuffer);
  }
  
  // Update all input types in sequence
  // Order matters: buttons first, then matrix, then encoders
  
  // Process direct pin buttons and shift register buttons
  updateButtons();
  
  // Scan button matrix and update encoder pin states for matrix encoders
  updateMatrix();
  
  // Process all encoder types (matrix, direct pin, and shift register)
  updateEncoders();
  
}