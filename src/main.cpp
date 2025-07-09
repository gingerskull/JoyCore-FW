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

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
                   32, 0, false, false, false,
                   false, false, false,
                   false, false, false);

void setup() {
  Joystick.begin();
  initButtonsFromLogical(logicalInputs, logicalInputCount);
  initEncodersFromLogical(logicalInputs, logicalInputCount);
  initMatrixFromLogical(logicalInputs, logicalInputCount);
}


void loop() {
  updateButtons();
  updateMatrix();
  updateEncoders();
  // printPin34567State();
}
