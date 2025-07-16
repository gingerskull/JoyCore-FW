// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GNGR-ButtonBox: Custom Button Box Controller
 * 
 * A versatile RP2040-based USB game controller supporting:
 * - Matrix button scanning
 * - Rotary encoders (matrix, direct pin, and shift register)
 * - Direct pin buttons
 * - 74HC165 shift register expansion
 * 
 * Appears as a standard 32-button USB gamepad for maximum compatibility.
 * 
 * Ported to RP2040 to take advantage of its dual-core architecture and PIO capabilities.
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
// RP2040 uses TinyUSB library for HID support
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
    // RP2040 USB setup via TinyUSB
    MyJoystick.begin();
    
    // Explicitly set hat switch to neutral position to prevent false readings
    MyJoystick.setHatSwitch(0, -1); // -1 = neutral position
    
    // Allow USB enumeration to complete
    delay(500);
    
    Serial.println("GNGR-ButtonBox RP2040 initialized");
}

void loop() {
    // Read shift register data for button updates
    if (shiftReg && shiftRegBuffer) {
        shiftReg->read(shiftRegBuffer);
    }
    
    // Update all input types in proper sequence
    updateButtons();   // Direct pin and shift register buttons
    updateMatrix();    // Button matrix and encoder pin states
    updateEncoders();  // All encoder types (now handles shift register reads internally for better timing)
    readUserAxes(MyJoystick); // Read all configured axes from user.h
    
    // Small delay to prevent overwhelming the USB bus
    // RP2040 can handle high update rates with its dual-core architecture
    delayMicroseconds(10);  // Ultra-fast polling with queue buffering
}