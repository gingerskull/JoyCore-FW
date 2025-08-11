// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"
#include "../../rp2040/JoystickWrapper.h"
#include "../../Config.h"
#include "../shift_register/ShiftRegister165.h"
#include <vector>

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
    std::vector<RuntimeLogicalButton> logicalButtons;
};

static std::vector<PinButtonGroup> pinGroups; // dynamic

struct ShiftRegButtonGroup {
    uint8_t regIndex = 0;
    uint8_t bitIndex = 0;
    std::vector<RuntimeLogicalButton> logicalButtons;
};

static std::vector<ShiftRegButtonGroup> shiftRegGroups; // dynamic

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
    
    pinGroups.clear();
    pinGroups.reserve(count);
    for (uint8_t i = 0; i < count; i++) {
        PinButtonGroup grp;
        grp.pin = configs[i].pin;
        grp.logicalButtons.resize(1);
        grp.logicalButtons[0].joyButtonID = configs[i].joyButtonID;
        grp.logicalButtons[0].behavior = configs[i].behavior;
        grp.logicalButtons[0].reverse = configs[i].reverse;
        pinMode(grp.pin, INPUT_PULLUP);
        bool physicalPressed = (digitalRead(grp.pin) == LOW);
        if (grp.logicalButtons[0].reverse) physicalPressed = !physicalPressed;
        grp.logicalButtons[0].lastState = physicalPressed;
        grp.logicalButtons[0].momentaryStartTime = 0;
        grp.logicalButtons[0].momentaryActive = false;
        pinGroups.push_back(grp);
    }
}

void updateButtons() {
    // Update direct pin buttons
    uint32_t now = millis();
    for (auto &group : pinGroups) {
        bool currentPhysicalState = digitalRead(group.pin);
        bool physicalPressed = (currentPhysicalState == LOW);
        for (auto &logicalBtn : group.logicalButtons) {
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
    for (auto &group : shiftRegGroups) {
        
        if (group.regIndex >= SHIFTREG_COUNT || group.bitIndex >= 8) continue;
        
        bool physicalPressed = (shiftRegBuffer[group.regIndex] & (1 << group.bitIndex)) == 0; // 74HC165: LOW = pressed
        
        // Process all logical buttons for this shift register bit
        for (auto &logicalBtn : group.logicalButtons) {
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
    pinGroups.clear();
    if (count == 0) return;
    
    // Count unique pins
    uint8_t uniquePins[64]; // Increased temporary capacity
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
    
    // Initialize pin groups
    pinGroups.reserve(uniquePinCount);
    for (uint8_t pinIdx = 0; pinIdx < uniquePinCount; pinIdx++) {
        PinButtonGroup grp;
        grp.pin = uniquePins[pinIdx];
        
        // Initialize pin once
        pinMode(uniquePins[pinIdx], INPUT_PULLUP);
        bool physicalPressed = digitalRead(uniquePins[pinIdx]) == LOW;
        
        // Count logical buttons for this pin
        uint8_t logicalCnt = 0;
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (isRegularButton(logicals[i]) && logicals[i].u.pin.pin == uniquePins[pinIdx]) {
                logicalCnt++;
            }
        }
        grp.logicalButtons.resize(logicalCnt);
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0; i < logicalCount && btnIdx < logicalCnt; ++i) {
            if (isRegularButton(logicals[i]) && logicals[i].u.pin.pin == uniquePins[pinIdx]) {
                grp.logicalButtons[btnIdx].joyButtonID = logicals[i].u.pin.joyButtonID;
                grp.logicalButtons[btnIdx].behavior = logicals[i].u.pin.behavior;
                grp.logicalButtons[btnIdx].reverse = logicals[i].u.pin.reverse;
                
                // Initialize lastState with effective pressed state
                bool effectivePressed = physicalPressed;
                if (grp.logicalButtons[btnIdx].reverse) {
                    effectivePressed = !effectivePressed;
                }
                grp.logicalButtons[btnIdx].lastState = effectivePressed;
                grp.logicalButtons[btnIdx].momentaryStartTime = 0;
                grp.logicalButtons[btnIdx].momentaryActive = false;
                
                btnIdx++;
            }
        }
        pinGroups.push_back(grp);
    }
}

void initShiftRegisterIfNeeded(const LogicalInput* logicals, uint8_t logicalCount) {
    // Reset
    shiftRegGroups.clear();
    
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
    ShiftRegPosition uniquePositions[128]; // temp storage
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
    
    // Initialize shift register groups
    shiftRegGroups.reserve(uniquePositionCount);
    for (uint8_t posIdx = 0; posIdx < uniquePositionCount; posIdx++) {
        ShiftRegButtonGroup grp;
        grp.regIndex = uniquePositions[posIdx].regIndex;
        grp.bitIndex = uniquePositions[posIdx].bitIndex;
        
        // Count logical buttons for this position
        uint8_t logicalCnt = 0;
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == grp.regIndex &&
                logicals[i].u.shiftreg.bitIndex == grp.bitIndex) {
                logicalCnt++;
            }
        }
        grp.logicalButtons.resize(logicalCnt);
        
        // Fill logical buttons
        uint8_t btnIdx = 0;
        for (uint8_t i = 0; i < logicalCount && btnIdx < logicalCnt; ++i) {
            if (logicals[i].type == INPUT_SHIFTREG && 
                logicals[i].u.shiftreg.behavior != ENC_A && 
                logicals[i].u.shiftreg.behavior != ENC_B &&
                logicals[i].u.shiftreg.regIndex == grp.regIndex &&
                logicals[i].u.shiftreg.bitIndex == grp.bitIndex) {
                grp.logicalButtons[btnIdx].joyButtonID = logicals[i].u.shiftreg.joyButtonID;
                grp.logicalButtons[btnIdx].behavior = logicals[i].u.shiftreg.behavior;
                grp.logicalButtons[btnIdx].reverse = logicals[i].u.shiftreg.reverse;
                grp.logicalButtons[btnIdx].lastState = false; // Initialize to not pressed
                grp.logicalButtons[btnIdx].momentaryStartTime = 0;
                grp.logicalButtons[btnIdx].momentaryActive = false;
                btnIdx++;
            }
        }
        shiftRegGroups.push_back(grp);
    }
}

// Debug helpers
uint16_t getButtonPinGroupCount() { return static_cast<uint16_t>(pinGroups.size()); }
uint16_t getShiftRegGroupCount() { return static_cast<uint16_t>(shiftRegGroups.size()); }
