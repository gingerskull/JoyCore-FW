// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"
#include "JoystickWrapper.h"
#include "Config.h"
#include "ShiftRegister165.h"

// Static variables for button management
struct PinLogicalButton {
    uint8_t joyButtonID;
    ButtonBehavior behavior;
    uint8_t reverse;
    bool lastState;
};

struct PinButtonGroup {
    uint8_t pin;
    PinLogicalButton* logicalButtons;
    uint8_t count;
};

static PinButtonGroup* pinGroups = nullptr;
static uint8_t pinGroupCount = 0;

// Shift register logical button support
struct ShiftRegLogicalButton {
    uint8_t joyButtonID;
    ButtonBehavior behavior;
    uint8_t reverse;
    bool lastState;
};

struct ShiftRegButtonGroup {
    uint8_t regIndex;
    uint8_t bitIndex;
    ShiftRegLogicalButton* logicalButtons;
    uint8_t count;
};

static ShiftRegButtonGroup* shiftRegGroups = nullptr;
static uint8_t shiftRegGroupCount = 0;

// Global shift register components
ShiftRegister165* shiftReg = nullptr;
uint8_t* shiftRegBuffer = nullptr;

void initButtons(const ButtonConfig* configs, uint8_t count) {
    // This function is deprecated - use initButtonsFromLogical instead
    // Keeping for backward compatibility
    
    // Clean up previous allocations
    if (pinGroups) {
        for (uint8_t i = 0; i < pinGroupCount; i++) {
            delete[] pinGroups[i].logicalButtons;
        }
        delete[] pinGroups;
    }
    
    pinGroupCount = count;
    pinGroups = new PinButtonGroup[count];
    
    for (uint8_t i = 0; i < count; i++) {
        pinGroups[i].pin = configs[i].pin;
        pinGroups[i].count = 1;
        pinGroups[i].logicalButtons = new PinLogicalButton[1];
        pinGroups[i].logicalButtons[0].joyButtonID = configs[i].joyButtonID;
        pinGroups[i].logicalButtons[0].behavior = configs[i].behavior;
        pinGroups[i].logicalButtons[0].reverse = configs[i].reverse;
        
        pinMode(pinGroups[i].pin, INPUT_PULLUP);
        pinGroups[i].logicalButtons[0].lastState = digitalRead(pinGroups[i].pin) == LOW;
        if (pinGroups[i].logicalButtons[0].reverse) {
            pinGroups[i].logicalButtons[0].lastState = !pinGroups[i].logicalButtons[0].lastState;
        }
    }
}

void updateButtons() {
    // Update direct pin buttons
    for (uint8_t groupIdx = 0; groupIdx < pinGroupCount; groupIdx++) {
        PinButtonGroup& group = pinGroups[groupIdx];
        bool currentPhysicalState = digitalRead(group.pin);
        bool physicalPressed = (currentPhysicalState == LOW);
        
        // Process all logical buttons for this pin
        for (uint8_t btnIdx = 0; btnIdx < group.count; btnIdx++) {
            PinLogicalButton& logicalBtn = group.logicalButtons[btnIdx];
            
            bool wasPressed = logicalBtn.lastState;
            bool isPressed = physicalPressed;
            
            // Apply reverse logic if enabled
            if (logicalBtn.reverse) {
                isPressed = !isPressed;
                // Don't reverse wasPressed - it's already stored in effective form
            }
            
            uint8_t joyIdx = (logicalBtn.joyButtonID > 0) ? (logicalBtn.joyButtonID - 1) : 0;
            
            switch (logicalBtn.behavior) {
                case NORMAL:
                    MyJoystick.setButton(joyIdx, isPressed);
                    break;
                    
                case MOMENTARY:
                    if (!wasPressed && isPressed) {
                        MyJoystick.setButton(joyIdx, true);
                        delay(50);
                        MyJoystick.setButton(joyIdx, false);
                    }
                    break;
                    
                default:
                    break;
            }
            
            // Update lastState (store the effective state, already reversed if needed)
            bool effectivePressed = physicalPressed;
            if (logicalBtn.reverse) {
                effectivePressed = !effectivePressed;
            }
            logicalBtn.lastState = effectivePressed;
        }
    }
    
    // Update shift register buttons if present
    updateShiftRegisterButtons();
}

void updateShiftRegisterButtons() {
    if (!shiftReg || !shiftRegBuffer) return;
    
    // Update shift register groups
    for (uint8_t groupIdx = 0; groupIdx < shiftRegGroupCount; groupIdx++) {
        ShiftRegButtonGroup& group = shiftRegGroups[groupIdx];
        
        if (group.regIndex >= SHIFTREG_COUNT) continue;
        
        bool physicalPressed = (shiftRegBuffer[group.regIndex] & (1 << group.bitIndex)) == 0; // 74HC165: LOW = pressed
        
        // Process all logical buttons for this shift register bit
        for (uint8_t btnIdx = 0; btnIdx < group.count; btnIdx++) {
            ShiftRegLogicalButton& logicalBtn = group.logicalButtons[btnIdx];
            
            bool pressed = physicalPressed;
            
            // Apply reverse logic if enabled
            if (logicalBtn.reverse) {
                pressed = !pressed;
            }
            
            uint8_t joyIdx = (logicalBtn.joyButtonID > 0) ? (logicalBtn.joyButtonID - 1) : 0;
            
            switch (logicalBtn.behavior) {
                case NORMAL:
                    MyJoystick.setButton(joyIdx, pressed);
                    break;
                    
                case MOMENTARY:
                    if (!logicalBtn.lastState && pressed) {
                        MyJoystick.setButton(joyIdx, true);
                        delay(50);
                        MyJoystick.setButton(joyIdx, false);
                    }
                    break;
                    
                default:
                    break;
            }
            
            logicalBtn.lastState = pressed;
        }
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
    return (input.type == INPUT_PIN && 
            input.u.pin.behavior != ENC_A && 
            input.u.pin.behavior != ENC_B);
}

void initRegularButtons(const LogicalInput* logicals, uint8_t logicalCount, uint8_t count) {
    // Clean up previous allocations
    if (pinGroups) {
        for (uint8_t i = 0; i < pinGroupCount; i++) {
            delete[] pinGroups[i].logicalButtons;
        }
        delete[] pinGroups;
    }
    
    if (count == 0) {
        pinGroups = nullptr;
        pinGroupCount = 0;
        return;
    }
    
    // Count unique pins
    uint8_t uniquePins[32]; // Assume max 32 pins
    uint8_t uniquePinCount = 0;
    
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (isRegularButton(logicals[i])) {
            uint8_t pin = logicals[i].u.pin.pin;
            bool found = false;
            for (uint8_t j = 0; j < uniquePinCount; j++) {
                if (uniquePins[j] == pin) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                uniquePins[uniquePinCount++] = pin;
            }
        }
    }
    
    pinGroupCount = uniquePinCount;
    pinGroups = new PinButtonGroup[pinGroupCount];
    
    // Initialize pin groups
    for (uint8_t pinIdx = 0; pinIdx < pinGroupCount; pinIdx++) {
        pinGroups[pinIdx].pin = uniquePins[pinIdx];
        pinGroups[pinIdx].count = 0;
        
        // Initialize pin once
        pinMode(uniquePins[pinIdx], INPUT_PULLUP);
        bool physicalPressed = digitalRead(uniquePins[pinIdx]) == LOW;
        
        // Count logical buttons for this pin
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (isRegularButton(logicals[i]) && logicals[i].u.pin.pin == uniquePins[pinIdx]) {
                pinGroups[pinIdx].count++;
            }
        }
        
        // Allocate logical buttons array
        pinGroups[pinIdx].logicalButtons = new PinLogicalButton[pinGroups[pinIdx].count];
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (isRegularButton(logicals[i]) && logicals[i].u.pin.pin == uniquePins[pinIdx]) {
                pinGroups[pinIdx].logicalButtons[btnIdx].joyButtonID = logicals[i].u.pin.joyButtonID;
                pinGroups[pinIdx].logicalButtons[btnIdx].behavior = logicals[i].u.pin.behavior;
                pinGroups[pinIdx].logicalButtons[btnIdx].reverse = logicals[i].u.pin.reverse;
                
                // Initialize lastState with effective pressed state
                bool effectivePressed = physicalPressed;
                if (pinGroups[pinIdx].logicalButtons[btnIdx].reverse) {
                    effectivePressed = !effectivePressed;
                }
                pinGroups[pinIdx].logicalButtons[btnIdx].lastState = effectivePressed;
                
                btnIdx++;
            }
        }
    }
}

void initShiftRegisterIfNeeded(const LogicalInput* logicals, uint8_t logicalCount) {
    // Clean up previous allocations
    if (shiftRegGroups) {
        for (uint8_t i = 0; i < shiftRegGroupCount; i++) {
            delete[] shiftRegGroups[i].logicalButtons;
        }
        delete[] shiftRegGroups;
        shiftRegGroups = nullptr;
        shiftRegGroupCount = 0;
    }
    
    // Check if any shift register inputs are present
    bool hasShiftReg = false;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_SHIFTREG) {
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
    
    // Count unique shift register positions (regIndex, bitIndex pairs)
    struct ShiftRegPosition {
        uint8_t regIndex;
        uint8_t bitIndex;
    };
    ShiftRegPosition uniquePositions[64]; // Assume max 64 positions
    uint8_t uniquePositionCount = 0;
    
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_SHIFTREG && 
            logicals[i].u.shiftreg.behavior != ENC_A && 
            logicals[i].u.shiftreg.behavior != ENC_B) {
            
            uint8_t reg = logicals[i].u.shiftreg.regIndex;
            uint8_t bit = logicals[i].u.shiftreg.bitIndex;
            
            bool found = false;
            for (uint8_t j = 0; j < uniquePositionCount; j++) {
                if (uniquePositions[j].regIndex == reg && uniquePositions[j].bitIndex == bit) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                uniquePositions[uniquePositionCount].regIndex = reg;
                uniquePositions[uniquePositionCount].bitIndex = bit;
                uniquePositionCount++;
            }
        }
    }
    
    if (uniquePositionCount == 0) return;
    
    shiftRegGroupCount = uniquePositionCount;
    shiftRegGroups = new ShiftRegButtonGroup[shiftRegGroupCount];
    
    // Initialize shift register groups
    for (uint8_t posIdx = 0; posIdx < shiftRegGroupCount; posIdx++) {
        shiftRegGroups[posIdx].regIndex = uniquePositions[posIdx].regIndex;
        shiftRegGroups[posIdx].bitIndex = uniquePositions[posIdx].bitIndex;
        shiftRegGroups[posIdx].count = 0;
        
        // Count logical buttons for this position
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == uniquePositions[posIdx].regIndex &&
                logicals[i].u.shiftreg.bitIndex == uniquePositions[posIdx].bitIndex) {
                shiftRegGroups[posIdx].count++;
            }
        }
        
        // Allocate logical buttons array
        shiftRegGroups[posIdx].logicalButtons = new ShiftRegLogicalButton[shiftRegGroups[posIdx].count];
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == uniquePositions[posIdx].regIndex &&
                logicals[i].u.shiftreg.bitIndex == uniquePositions[posIdx].bitIndex) {
                
                shiftRegGroups[posIdx].logicalButtons[btnIdx].joyButtonID = logicals[i].u.shiftreg.joyButtonID;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].behavior = logicals[i].u.shiftreg.behavior;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].reverse = logicals[i].u.shiftreg.reverse;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].lastState = false; // Initialize to not pressed
                
                btnIdx++;
            }
        }
    }
}
