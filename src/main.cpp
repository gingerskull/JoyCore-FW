/*
 * JoyCore-FW: RP2040 USB Game Controller
 *
 * Overview of supported input systems (enable/configure in ConfigDigital.h and ConfigAxis.h):
 * - Direct pin buttons (pins 4–13 in current config; INPUT_PULLUP, LOW = pressed)
 * - 74HC165 shift-register inputs (active-low; current config uses 2 chained parts = 16 inputs)
 * - Matrix button scanning (available; disabled until BTN_ROW/BTN_COL and INPUT_MATRIX are configured)
 * - Rotary encoders (from direct pins, matrix pins, or shift-register bits; ENC_A then ENC_B ordering)
 * - Analog axes with deadband, filtering (EWMA/adaptive), curves, and unified HID scaling
 *
 * USB HID notes:
 * - rp2040-HID provides a descriptor with up to 128 buttons and 16 axes. This firmware configures 32 buttons
 *   and up to 8 axes via the Joystick_ wrapper; unused descriptor fields are simply not updated.
 *
 * Runtime order and timing:
 * - Shift registers are read periodically into a shared buffer (shiftRegBuffer) and never on-demand by encoders
 *   to avoid contention. Throttling depends on SHIFTREG_COUNT.
 * - Update order: buttons -> matrix -> encoders -> axes -> send HID. Encoders depend on current matrix/shift-reg
 *   states, so this order ensures consistent behavior.
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


 // USB joystick configuration: exposes 32 buttons, 1 hat switch, and 8 axes to the wrapper.
 // rp2040-HID under the hood supports up to 128 buttons and 16 axes; unused fields remain untouched.
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
    
    // Read shift-register chain into the shared buffer at a controlled rate.
    // Encoders consume this buffer but do NOT trigger reads themselves to avoid conflicts.
    if (shiftReg && shiftRegBuffer) {
        // Read every 5 ms for 2+ registers, every 1 ms for a single register (stability versus latency)
        uint32_t readInterval = (SHIFTREG_COUNT > 1) ? 5 : 1;
        if (currentTime - lastShiftRegRead >= readInterval) {
            shiftReg->read(shiftRegBuffer);
            lastShiftRegRead = currentTime;
        }
    }
    
    // Update in dependency-safe order:
    updateButtons();   // Direct pins and shift-register buttons (uses shiftRegBuffer)
    updateMatrix();    // Matrix scanning (updates matrix state used by encoders)
    updateEncoders();  // Rotary encoders (uses current matrix/shift-register states)
    
    // Read axis values
    readUserAxes(MyJoystick);
    
    // Send all HID updates to computer
    MyJoystick.sendState();
}
