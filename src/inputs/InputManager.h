// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "buttons/ButtonInput.h"
#include "buttons/MatrixInput.h"
#include "encoders/EncoderInput.h"
#include "ShiftRegisterManager.h"
#include "../rp2040/JoystickWrapper.h"

class InputManager {
public:
    void begin(const LogicalInput* inputs, uint8_t count);
    void update(Joystick_ &js);
private:
    bool _begun = false;
};

extern InputManager g_inputManager;
