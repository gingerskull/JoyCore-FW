// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GNGR-ButtonBox: Custom Button Box Controller
 * 
 * A versatile Arduino-based USB game controller supporting:
 * - Matrix button scanning
 * - Rotary encoders (matrix, direct pin, and shift register)
 * - Direct pin buttons
 * - 74HC165 shift register expansion
 * 
 * Appears as a standard 32-button USB gamepad for maximum compatibility.
 */

#include <Arduino.h>
#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"
#include "MatrixInput.h"
#include "ShiftRegister165.h"
#include "ConfigAxis.h"

// Global AnalogAxisManager to reduce stack usage
AnalogAxisManager g_axisManager;

// USB joystick configuration: minimal configuration for maximum RAM savings
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                   16, 0, true, true, false,
                   false, false, false,
                   false, false);

// External shift register components
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
    
    // Initialize all input subsystems from user configuration
    initButtonsFromLogical(logicalInputs, logicalInputCount);
    initEncodersFromLogical(logicalInputs, logicalInputCount);
    initMatrixFromLogical(logicalInputs, logicalInputCount);

    // Configure all user-defined axes
    setupUserAxes(Joystick);

    // Initialize USB joystick interface
    Joystick.begin();
    delay(1000);
}

void loop() {
    // Read shift register data once per loop for consistency
    if (shiftReg && shiftRegBuffer) {
        shiftReg->read(shiftRegBuffer);
    }
    
    // Update all input types in proper sequence
    updateButtons();   // Direct pin and shift register buttons
    updateMatrix();    // Button matrix and encoder pin states
    updateEncoders();  // All encoder types
    readUserAxes(Joystick); // Read all configured axes from user.h
}