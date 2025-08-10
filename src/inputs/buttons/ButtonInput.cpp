// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"
#include "../../rp2040/JoystickWrapper.h"
#include "../../Config.h"
#include "../shift_register/ShiftRegister165.h"
#include "../PoolConfig.h"

// Unified runtime logical button state used for all digital sources
struct RuntimeLogicalButton {
    uint8_t joyButtonID = 0;
    ButtonBehavior behavior = NORMAL;
    uint8_t reverse = 0;
    bool lastState = false;
    uint32_t momentaryStartTime = 0;  // Non-blocking timing
    bool momentaryActive = false;     // Active pulse flag
};

struct PinButtonGroup {
    uint8_t pin = 0;
    RuntimeLogicalButton logicalButtons[MAX_LOGICAL_PER_PIN];
    uint8_t count = 0; // active logical buttons
};

static PinButtonGroup pinGroups[MAX_BUTTON_PIN_GROUPS];
static uint8_t pinGroupCount = 0; // active pin groups

struct ShiftRegButtonGroup {
    uint8_t regIndex = 0;
    uint8_t bitIndex = 0;
    RuntimeLogicalButton logicalButtons[MAX_LOGICAL_PER_SHIFT_BIT];
    uint8_t count = 0; // active logical buttons
};

static ShiftRegButtonGroup shiftRegGroups[MAX_SHIFTREG_GROUPS];
static uint8_t shiftRegGroupCount = 0; // active shift-register groups

// Shared helper for NORMAL / MOMENTARY behaviors (non-blocking)
static inline void processLogicalButton(uint32_t now, bool pressedPhysical, uint8_t joyButtonID, ButtonBehavior behavior, bool reverse, bool &lastState, uint32_t &pulseStart, bool &pulseActive) {
    bool effectivePressed = pressedPhysical;
    if (reverse) effectivePressed = !effectivePressed;
    uint8_t joyIdx = (joyButtonID > 0) ? (joyButtonID - 1) : 0;
    switch(behavior) {
        case NORMAL:
            MyJoystick.setButton(joyIdx, effectivePressed);
            break;
        case MOMENTARY:
            if(!lastState && effectivePressed && !pulseActive) {
                MyJoystick.setButton(joyIdx, 1);
                pulseStart = now;
                pulseActive = true;
            }
            if(pulseActive && (now - pulseStart) >= 50) { // 50 ms pulse
                MyJoystick.setButton(joyIdx, 0);
                pulseActive = false;
            }
            break;
        case ENC_A:
        case ENC_B:
            // Handled elsewhere
            break;
    }
    lastState = effectivePressed;
}

// Global shift register components
ShiftRegister165* shiftReg = nullptr; // kept as pointer (library object may allocate)
static uint8_t shiftRegRawBuffer[SHIFTREG_COUNT];
uint8_t* shiftRegBuffer = shiftRegRawBuffer; // legacy external reference if used elsewhere

void initButtons(const ButtonConfig* configs, uint8_t count) {
    // This function is deprecated - use initButtonsFromLogical instead
    // Keeping for backward compatibility
    
    pinGroupCount = (count > MAX_BUTTON_PIN_GROUPS) ? MAX_BUTTON_PIN_GROUPS : count;
    for (uint8_t i = 0; i < pinGroupCount; i++) {
        pinGroups[i].pin = configs[i].pin;
        pinGroups[i].count = 1;
        pinGroups[i].logicalButtons[0].joyButtonID = configs[i].joyButtonID;
        pinGroups[i].logicalButtons[0].behavior = configs[i].behavior;
        pinGroups[i].logicalButtons[0].reverse = configs[i].reverse;
        pinMode(pinGroups[i].pin, INPUT_PULLUP);
        bool physicalPressed = (digitalRead(pinGroups[i].pin) == LOW);
        if (pinGroups[i].logicalButtons[0].reverse) physicalPressed = !physicalPressed;
        pinGroups[i].logicalButtons[0].lastState = physicalPressed;
        pinGroups[i].logicalButtons[0].momentaryStartTime = 0;
        pinGroups[i].logicalButtons[0].momentaryActive = false;
    }
}

void updateButtons() {
    // Update direct pin buttons
    uint32_t now = millis();
    for (uint8_t groupIdx = 0; groupIdx < pinGroupCount; groupIdx++) {
        PinButtonGroup& group = pinGroups[groupIdx];
        bool currentPhysicalState = digitalRead(group.pin);
        bool physicalPressed = (currentPhysicalState == LOW);
        for (uint8_t btnIdx = 0; btnIdx < group.count; btnIdx++) {
            RuntimeLogicalButton& logicalBtn = group.logicalButtons[btnIdx];
            processLogicalButton(now, physicalPressed, logicalBtn.joyButtonID, logicalBtn.behavior, logicalBtn.reverse, logicalBtn.lastState, logicalBtn.momentaryStartTime, logicalBtn.momentaryActive);
        }
    }
    
    // Update shift register buttons if present
    updateShiftRegisterButtons();
}

void updateShiftRegisterButtons() {
    if (!shiftReg || !shiftRegBuffer) return;
    uint32_t now = millis();
    
    // Update shift register groups
    for (uint8_t groupIdx = 0; groupIdx < shiftRegGroupCount; groupIdx++) {
        ShiftRegButtonGroup& group = shiftRegGroups[groupIdx];
        
        if (group.regIndex >= SHIFTREG_COUNT || group.bitIndex >= 8) continue;
        
        bool physicalPressed = (shiftRegBuffer[group.regIndex] & (1 << group.bitIndex)) == 0; // 74HC165: LOW = pressed
        
        // Process all logical buttons for this shift register bit
        for (uint8_t btnIdx = 0; btnIdx < group.count; btnIdx++) {
            RuntimeLogicalButton& logicalBtn = group.logicalButtons[btnIdx];
            processLogicalButton(now, physicalPressed, logicalBtn.joyButtonID, logicalBtn.behavior, logicalBtn.reverse, logicalBtn.lastState, logicalBtn.momentaryStartTime, logicalBtn.momentaryActive);
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
    // Reset
    pinGroupCount = 0;
    if (count == 0) return;
    
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
    
    pinGroupCount = (uniquePinCount > MAX_BUTTON_PIN_GROUPS) ? MAX_BUTTON_PIN_GROUPS : uniquePinCount;
    
    // Initialize pin groups
    for (uint8_t pinIdx = 0; pinIdx < pinGroupCount; pinIdx++) {
        pinGroups[pinIdx].pin = uniquePins[pinIdx];
    pinGroups[pinIdx].count = 0;
        
        // Initialize pin once
        pinMode(uniquePins[pinIdx], INPUT_PULLUP);
        bool physicalPressed = digitalRead(uniquePins[pinIdx]) == LOW;
        
        // Count logical buttons (bounded)
        for (uint8_t i = 0; i < logicalCount && pinGroups[pinIdx].count < MAX_LOGICAL_PER_PIN; ++i) {
            if (isRegularButton(logicals[i]) && logicals[i].u.pin.pin == uniquePins[pinIdx]) {
                pinGroups[pinIdx].count++;
            }
        }
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0, btnIdx = 0; i < logicalCount && btnIdx < pinGroups[pinIdx].count; ++i) {
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
                pinGroups[pinIdx].logicalButtons[btnIdx].momentaryStartTime = 0;
                pinGroups[pinIdx].logicalButtons[btnIdx].momentaryActive = false;
                
                btnIdx++;
            }
        }
    }
}

void initShiftRegisterIfNeeded(const LogicalInput* logicals, uint8_t logicalCount) {
    // Reset
    shiftRegGroupCount = 0;
    
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
        if (!shiftReg) { // single instance
            shiftReg = new ShiftRegister165(plPin, clkPin, qhPin, SHIFTREG_COUNT);
            shiftReg->begin();
        }
        for (uint8_t i = 0; i < SHIFTREG_COUNT; i++) shiftRegRawBuffer[i] = 0xFF;
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
    
    shiftRegGroupCount = (uniquePositionCount > MAX_SHIFTREG_GROUPS) ? MAX_SHIFTREG_GROUPS : uniquePositionCount;
    
    // Initialize shift register groups
    for (uint8_t posIdx = 0; posIdx < shiftRegGroupCount; posIdx++) {
        shiftRegGroups[posIdx].regIndex = uniquePositions[posIdx].regIndex;
        shiftRegGroups[posIdx].bitIndex = uniquePositions[posIdx].bitIndex;
    shiftRegGroups[posIdx].count = 0;
        
        // Count logical buttons (bounded)
        for (uint8_t i = 0; i < logicalCount && shiftRegGroups[posIdx].count < MAX_LOGICAL_PER_SHIFT_BIT; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == uniquePositions[posIdx].regIndex &&
                logicals[i].u.shiftreg.bitIndex == uniquePositions[posIdx].bitIndex) {
                shiftRegGroups[posIdx].count++;
            }
        }
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0; i < logicalCount && btnIdx < shiftRegGroups[posIdx].count; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == uniquePositions[posIdx].regIndex &&
                logicals[i].u.shiftreg.bitIndex == uniquePositions[posIdx].bitIndex) {
                shiftRegGroups[posIdx].logicalButtons[btnIdx].joyButtonID = logicals[i].u.shiftreg.joyButtonID;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].behavior = logicals[i].u.shiftreg.behavior;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].reverse = logicals[i].u.shiftreg.reverse;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].lastState = false; // Initialize to not pressed
                shiftRegGroups[posIdx].logicalButtons[btnIdx].momentaryStartTime = 0;
                shiftRegGroups[posIdx].logicalButtons[btnIdx].momentaryActive = false;
                btnIdx++;
            }
        }
    }
}
