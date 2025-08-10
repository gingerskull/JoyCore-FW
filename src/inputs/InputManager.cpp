// SPDX-License-Identifier: GPL-3.0-or-later
#include "InputManager.h"
#include "../config/ConfigAxis.h" // brings in readUserAxes definition

// Forward declare in case inclusion order changes
inline void readUserAxes(Joystick_& joystick);

InputManager g_inputManager;

// External shift register symbols
extern ShiftRegister165* shiftReg;
extern uint8_t* shiftRegBuffer;

void InputManager::begin(const LogicalInput* inputs, uint8_t count) {
    if (_begun) return;
    initButtonsFromLogical(inputs, count);
    initEncodersFromLogical(inputs, count);
    initMatrixFromLogical(inputs, count);
    if (shiftReg && shiftRegBuffer) {
        g_shiftRegisterManager.begin(shiftReg, shiftRegBuffer, SHIFTREG_COUNT);
    }
    _begun = true;
}

void InputManager::update(Joystick_ &js) {
    if (!_begun) return;
    uint32_t now = millis();
    g_shiftRegisterManager.update(now);
    updateButtons();
    updateMatrix();
    updateEncoders();
    readUserAxes(js);
    js.sendState();
}
