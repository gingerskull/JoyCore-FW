// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GNGR-ButtonBox: Custom Button Box Controller
 * 
 * A versatile Teensy 4.0-based USB game controller supporting:
 * - Matrix button scanning
 * - Rotary encoders (matrix, direct pin, and shift register)
 * - Direct pin buttons
 * - 74HC165 shift register expansion
 * 
 * Appears as a standard 32-button USB gamepad for maximum compatibility.
 * 
 * Ported to Teensy 4.0 to take advantage of its superior USB HID support.
 */

#include <Arduino.h>
#include "Config.h"
#define DEFINE_MYJOYSTICK
#include "JoystickWrapper.h"
#include "ButtonInput.h"
#include "EncoderInput.h"
#include "MatrixInput.h"
#include "ShiftRegister165.h"
#include "ConfigAxis.h"


// USB joystick configuration: 32 buttons, 6 analog axes, 1 hat switch
// Teensy 4.0 handles USB descriptors automatically
Joystick_ MyJoystick(0x03, 0x04, 32, 0, true, true, true, true, true, true, true, true);

// External shift register components
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
    // Initialize serial for debugging (optional)
    Serial.begin(115200);
    
    // Initialize all input subsystems from user configuration
    initButtonsFromLogical(logicalInputs, logicalInputCount);
    initEncodersFromLogical(logicalInputs, logicalInputCount);
    initMatrixFromLogical(logicalInputs, logicalInputCount);

    // Configure all user-defined axes
    setupUserAxes(MyJoystick);

    // Initialize USB joystick interface
    // Teensy 4.0 USB setup is much simpler than Arduino Leonardo
    MyJoystick.begin();
    
    // Explicitly set hat switch to neutral position to prevent false readings
    MyJoystick.setHatSwitch(0, -1); // -1 = neutral position
    
    // Shorter delay since Teensy's USB is more reliable
    delay(500);
    
    Serial.println("GNGR-ButtonBox Teensy 4.0 initialized");
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
    readUserAxes(MyJoystick); // Read all configured axes from user.h
    
    // Small delay to prevent overwhelming the USB bus
    // Teensy 4.0 can handle higher update rates, but this is conservative
    //delayMicroseconds(100);
}