// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include "ShiftRegister165.h"

// Static variables for button management
static ButtonConfig* buttons = nullptr;
static uint8_t btnCount = 0;
static bool* lastStates = nullptr;

// Global shift register components
ShiftRegister165* shiftReg = nullptr;
uint8_t* shiftRegBuffer = nullptr;

void initButtons(const ButtonConfig* configs, uint8_t count) {
    btnCount = count;
    
    // Clean up previous allocations
    delete[] buttons;
    delete[] lastStates;
    
    // Allocate and copy configuration data
    buttons = new ButtonConfig[count];
    lastStates = new bool[count];
    
    for (uint8_t i = 0; i < count; i++) {
        buttons[i] = configs[i];
        pinMode(buttons[i].pin, INPUT_PULLUP);
        lastStates[i] = digitalRead(buttons[i].pin);
    }
}

void updateButtons() {
    // Update direct pin buttons
    for (uint8_t i = 0; i < btnCount; i++) {
        bool currentState = digitalRead(buttons[i].pin);
        bool wasPressed = (lastStates[i] == LOW);
        bool isPressed = (currentState == LOW);
        
        uint8_t joyIdx = (buttons[i].joyButtonID > 0) ? (buttons[i].joyButtonID - 1) : 0;
        
        switch (buttons[i].behavior) {
            case NORMAL:
                Joystick.setButton(joyIdx, isPressed);
                break;
                
            case MOMENTARY:
                if (!wasPressed && isPressed) {
                    Joystick.setButton(joyIdx, true);
                    delay(10);
                    Joystick.setButton(joyIdx, false);
                }
                break;
                
            case ENC_A:
            case ENC_B:
                // Encoder behaviors are handled by EncoderInput
                break;
        }
        
        lastStates[i] = currentState;
    }
    
    // Update shift register buttons
    updateShiftRegisterButtons();
}

void updateShiftRegisterButtons() {
    if (!shiftReg || !shiftRegBuffer) return;
    
    for (uint8_t i = 0; i < logicalInputCount; ++i) {
        const LogicalInput& input = logicalInputs[i];
        
        if (input.source != SRC_SHIFTREG) continue;
        if (input.u.shiftreg.behavior == ENC_A || input.u.shiftreg.behavior == ENC_B) continue;
        
        uint8_t reg = input.u.shiftreg.regIndex;
        uint8_t bit = input.u.shiftreg.bitIndex;
        
        if (reg >= SHIFTREG_COUNT || bit >= 8) continue;
        
        bool pressed = (shiftRegBuffer[reg] & (1 << bit)) == 0; // 74HC165: LOW = pressed
        uint8_t joyIdx = (input.u.shiftreg.joyButtonID > 0) ? (input.u.shiftreg.joyButtonID - 1) : 0;
        
        Joystick.setButton(joyIdx, pressed);
    }
}

void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Count non-encoder buttons
    uint8_t count = 0;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (isRegularButton(logicals[i])) {
            count++;
        }
    }
    
    if (count > 0) {
        initRegularButtons(logicals, logicalCount, count);
    }
    
    initShiftRegisterIfNeeded(logicals, logicalCount);
}

bool isRegularButton(const LogicalInput& input) {
    return (input.type == LOGICAL_BTN && 
            input.u.btn.behavior != ENC_A && 
            input.u.btn.behavior != ENC_B &&
            !(input.source == SRC_SHIFTREG && 
              (input.u.shiftreg.behavior == ENC_A || input.u.shiftreg.behavior == ENC_B)));
}

void initRegularButtons(const LogicalInput* logicals, uint8_t logicalCount, uint8_t count) {
    ButtonConfig* configs = new ButtonConfig[count];
    uint8_t idx = 0;
    
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (isRegularButton(logicals[i])) {
            configs[idx].pin = logicals[i].u.btn.pin;
            configs[idx].joyButtonID = logicals[i].u.btn.joyButtonID;
            configs[idx].behavior = logicals[i].u.btn.behavior;
            idx++;
        }
    }
    
    initButtons(configs, count);
    delete[] configs;
}

void initShiftRegisterIfNeeded(const LogicalInput* logicals, uint8_t logicalCount) {
    // Check if any shift register inputs are present
    bool hasShiftReg = false;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].source == SRC_SHIFTREG) {
            hasShiftReg = true;
            break;
        }
    }
    
    if (!hasShiftReg) return;
    
    // Find shift register pins
    int plPin = -1, clkPin = -1, qhPin = -1;
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
        switch (hardwarePinMap[i].type) {
            case SHIFTREG_PL:  plPin = atoi(hardwarePinMap[i].name); break;
            case SHIFTREG_CLK: clkPin = atoi(hardwarePinMap[i].name); break;
            case SHIFTREG_QH:  qhPin = atoi(hardwarePinMap[i].name); break;
            default: break; // Handle all other pin types
        }
    }
    
    if (plPin >= 0 && clkPin >= 0 && qhPin >= 0) {
        shiftReg = new ShiftRegister165(plPin, clkPin, qhPin, SHIFTREG_COUNT);
        shiftReg->begin();
        delete[] shiftRegBuffer;
        shiftRegBuffer = new uint8_t[SHIFTREG_COUNT];
    }
}
