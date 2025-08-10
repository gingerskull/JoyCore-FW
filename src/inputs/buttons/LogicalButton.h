// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "../../rp2040/JoystickWrapper.h"
#include "../../Config.h"

// Unified runtime logical button state used for all digital sources
struct RuntimeLogicalButton {
    uint8_t joyButtonID = 0;
    ButtonBehavior behavior = NORMAL;
    uint8_t reverse = 0;          // If set, invert physical pressed logic
    bool lastState = false;       // Last effective pressed state
    uint32_t momentaryStartTime = 0;  // Start time for MOMENTARY pulse
    bool momentaryActive = false;     // True while pulse is active
};

// Shared helper implementing NORMAL and MOMENTARY semantics (non-blocking)
static inline void processLogicalButton(uint32_t now,
                                        bool physicalPressed,
                                        RuntimeLogicalButton &btn) {
    bool effectivePressed = physicalPressed;
    if (btn.reverse) effectivePressed = !effectivePressed;
    uint8_t joyIdx = (btn.joyButtonID > 0) ? (btn.joyButtonID - 1) : 0;
    switch (btn.behavior) {
        case NORMAL:
            MyJoystick.setButton(joyIdx, effectivePressed);
            break;
        case MOMENTARY:
            if(!btn.lastState && effectivePressed && !btn.momentaryActive) {
                MyJoystick.setButton(joyIdx, 1);
                btn.momentaryStartTime = now;
                btn.momentaryActive = true;
            }
            if(btn.momentaryActive && (now - btn.momentaryStartTime) >= 50) { // 50 ms pulse
                MyJoystick.setButton(joyIdx, 0);
                btn.momentaryActive = false;
            }
            break;
        case ENC_A:
        case ENC_B:
            // Encoder phases handled separately
            break;
    }
    btn.lastState = effectivePressed;
}
