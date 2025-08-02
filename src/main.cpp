// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * JoyCore-FW: RP2040 USB Game Controller
 * 
 * A versatile RP2040-based USB game controller supporting:
 * - Direct pin buttons (pins 4-13)
 * - 74HC165 shift register expansion (2 registers, 16 inputs)
 * - Matrix button scanning (currently disabled)
 * - Rotary encoders (via shift register or direct pins)
 * - Analog axes with advanced signal processing and filtering
 * 
 * Uses rp2040-HID library for native USB HID gamepad functionality.
 * System configuration is managed through ConfigDigital.h and ConfigAxis.h
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


// USB joystick configuration: 32 buttons, 1 hat switch, 8 analog axes
// rp2040-HID handles USB descriptors automatically
Joystick_ MyJoystick(0x03, 0x04, 32, 0, true, true, false, false, false, false, false, false);

// External shift register components
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
    // Initialize USB joystick interface EARLY for HID functionality
    MyJoystick.begin();
 
    // Explicitly set hat switch to neutral position
    MyJoystick.setHatSwitch(0, -1);
    
    // Initialize all input subsystems
    initButtonsFromLogical(logicalInputs, logicalInputCount);
    initEncodersFromLogical(logicalInputs, logicalInputCount);
    initMatrixFromLogical(logicalInputs, logicalInputCount);

    // Initialize axis system
    setupUserAxes(MyJoystick);
    
    // Delay for USB enumeration
    delay(500);
}

void loop() {
    static uint32_t lastShiftRegRead = 0;
    uint32_t currentTime = millis();
    
    // Read shift register data for button updates - throttle for stability with multiple registers
    if (shiftReg && shiftRegBuffer) {
        // Read shift registers every 5ms for 2+ registers, every 1ms for single register
        uint32_t readInterval = (SHIFTREG_COUNT > 1) ? 5 : 1;
        if (currentTime - lastShiftRegRead >= readInterval) {
            shiftReg->read(shiftRegBuffer);
            lastShiftRegRead = currentTime;
        }
    }
    
    // Update all input types in proper sequence
    updateButtons();   // Direct pin and shift register buttons
    updateMatrix();    // Button matrix scanning
    updateEncoders();  // All encoder types
    
    // Read axis values
    readUserAxes(MyJoystick);
    
    // Send all HID updates to computer
    MyJoystick.sendState();
}
