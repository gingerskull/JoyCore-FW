// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GNGR-ButtonBox: Custom Button Box Controller
 * 
 * A versatile RP2040 Raspberry Pi Pico-based USB game controller supporting:
 * - Matrix button scanning
 * - Rotary encoders (matrix, direct pin, and shift register)
 * - Direct pin buttons
 * - 74HC165 shift register expansion
 * - ADS1115 external ADC for high-resolution analog inputs
 * 
 * Appears as a standard USB gamepad for maximum compatibility.
 * 
 * Ported to RP2040 Raspberry Pi Pico using PicoGamepad library.
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


// USB joystick configuration: 32 buttons, 8 analog axes, 4 hat switches
// PicoGamepad handles USB descriptors automatically
Joystick_ MyJoystick(0x03, 0x04, 32, 0, true, true, false, false, false, false, false, false);

// External shift register components
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Initialize serial for debugging
    Serial.begin(115200);
    
    // Wait for serial connection on RP2040
    uint32_t serialWait = millis();
    while (!Serial && (millis() - serialWait < 2000)) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(100);
    }
    digitalWrite(LED_BUILTIN, LOW);
    
    Serial.println("GNGR-ButtonBox RP2040 Pico - Final Version");
    
    // Initialize USB joystick interface EARLY for HID functionality
    MyJoystick.begin();
    
    // Explicitly set hat switch to neutral position
    MyJoystick.setHatSwitch(0, -1);
    
    Serial.println("Joystick initialized. Initializing input systems...");
    
    // Initialize all input subsystems (but skip axis setup)
    initButtonsFromLogical(logicalInputs, logicalInputCount);
    initEncodersFromLogical(logicalInputs, logicalInputCount);
    initMatrixFromLogical(logicalInputs, logicalInputCount);

    // SKIP AXIS SETUP - This was causing the button interference
    // setupUserAxes(MyJoystick);  // DISABLED - was interfering with button pins
    
    // Delay for USB enumeration
    delay(500);
    
    Serial.println("GNGR-ButtonBox initialized successfully!");
    Serial.println("All button inputs should now work in HID applications.");
}

void loop() {
    // Read shift register data for button updates
    if (shiftReg && shiftRegBuffer) {
        shiftReg->read(shiftRegBuffer);
    }
    
    // Update all input types in proper sequence
    updateButtons();   // Direct pin and shift register buttons
    updateMatrix();    // Button matrix scanning
    updateEncoders();  // All encoder types
    
    // SKIP AXIS READING - No axes connected, and this was causing interference
    // readUserAxes(MyJoystick);  // DISABLED - was interfering with button pins
    
    // Send all HID updates to computer
    MyJoystick.sendState();
    
    // Small delay for optimal RP2040 performance
    delayMicroseconds(50);
}