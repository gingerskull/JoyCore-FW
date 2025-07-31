// SPDX-License-Identifier: GPL-3.0-or-later
#include "MatrixInput.h"
#include "Config.h"
#include "JoystickWrapper.h"
#include "ButtonMatrix.h"

// Dynamic matrix config
static uint8_t ROWS = 0;
static uint8_t COLS = 0;
static byte* rowPins = nullptr;
static byte* colPins = nullptr;
static char* keymap = nullptr;  // Changed from char** to char*
static ButtonMatrix* buttonMatrix = nullptr;

// Support multiple logical buttons per physical position
struct MatrixLogicalButton {
    uint8_t joyButtonID;
    ButtonBehavior behavior;
    uint8_t reverse;
    bool lastState;
    bool lastMomentaryState;
};

static MatrixLogicalButton** logicalButtons = nullptr; // Array of arrays
static uint8_t* logicalButtonCounts = nullptr; // Count per position

static bool* lastMomentaryStates = nullptr;

bool g_encoderMatrixPinStates[20] = {1}; // indexed by pin number, default HIGH

// Helper: compare numeric pin to pin name string
static bool pinEqualsName(uint8_t pin, const char* pinName) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", pin);
    return strcmp(buf, pinName) == 0;
}

void initMatrixFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Find max row/col and count matrix buttons
    uint8_t maxRow = 0, maxCol = 0, count = 0;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            if (logicals[i].u.matrix.row > maxRow) maxRow = logicals[i].u.matrix.row;
            if (logicals[i].u.matrix.col > maxCol) maxCol = logicals[i].u.matrix.col;
            count++;
        }
    }
    ROWS = maxRow + 1;
    COLS = maxCol + 1;

    // Allocate arrays
    if (rowPins) delete[] rowPins;
    if (colPins) delete[] colPins;
    if (keymap) delete[] keymap;
    if (logicalButtons) {
        for (uint8_t i = 0; i < ROWS * COLS; i++) {
            delete[] logicalButtons[i];
        }
        delete[] logicalButtons;
    }
    if (logicalButtonCounts) delete[] logicalButtonCounts;
    if (lastMomentaryStates) delete[] lastMomentaryStates;

    rowPins = new byte[ROWS];
    colPins = new byte[COLS];
    keymap = new char[ROWS * COLS];  // Flat array for keymap
    logicalButtons = new MatrixLogicalButton*[ROWS * COLS];
    logicalButtonCounts = new uint8_t[ROWS * COLS]();
    lastMomentaryStates = new bool[ROWS * COLS]{}; // All false

    // Fill row/col pins from hardwarePinMap, but exclude encoder pins
    uint8_t rowIdx = 0, colIdx = 0;
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
        HardwarePinName pinName = hardwarePinMap[i].name;
        // Check if this pin is used by an encoder
        bool isEncoderPin = false;
        for (uint8_t j = 0; j < logicalCount; ++j) {
            if (logicals[j].type == INPUT_PIN && 
                (logicals[j].u.pin.behavior == ENC_A || logicals[j].u.pin.behavior == ENC_B) &&
                pinEqualsName(logicals[j].u.pin.pin, pinName)) {
                isEncoderPin = true;
                break;
            }
        }
        // Only add to matrix if not used by encoder
        PinType type = getPinType(pinName);
        if (!isEncoderPin) {
            if (type == BTN_ROW && rowIdx < ROWS) rowPins[rowIdx++] = atoi(pinName); // store Arduino pin number
            if (type == BTN_COL && colIdx < COLS) colPins[colIdx++] = atoi(pinName);
        }
    }

    // Fill keymap
    for (uint8_t r = 0; r < ROWS; ++r)
        for (uint8_t c = 0; c < COLS; ++c)
            keymap[r * COLS + c] = 'A' + r * COLS + c; // unique char

    // Initialize logical button arrays
    for (uint8_t i = 0; i < ROWS * COLS; ++i) {
        logicalButtons[i] = nullptr;
        logicalButtonCounts[i] = 0;
    }

    // Count logical buttons per position
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t idx = logicals[i].u.matrix.row * COLS + logicals[i].u.matrix.col;
            logicalButtonCounts[idx]++;
        }
    }

    // Allocate logical button arrays
    for (uint8_t i = 0; i < ROWS * COLS; ++i) {
        if (logicalButtonCounts[i] > 0) {
            logicalButtons[i] = new MatrixLogicalButton[logicalButtonCounts[i]];
        }
    }

    // Reset counts for filling
    for (uint8_t i = 0; i < ROWS * COLS; ++i) {
        logicalButtonCounts[i] = 0;
    }

    // Fill logical button configurations
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t idx = logicals[i].u.matrix.row * COLS + logicals[i].u.matrix.col;
            uint8_t btnIdx = logicalButtonCounts[idx]++;
            
            logicalButtons[idx][btnIdx].joyButtonID = logicals[i].u.matrix.joyButtonID;
            logicalButtons[idx][btnIdx].behavior = logicals[i].u.matrix.behavior;
            logicalButtons[idx][btnIdx].reverse = logicals[i].u.matrix.reverse;
            logicalButtons[idx][btnIdx].lastState = false;
            logicalButtons[idx][btnIdx].lastMomentaryState = false;
        }
    }

    // Create button matrix - pass the flat keymap directly
    buttonMatrix = new ButtonMatrix(keymap, rowPins, colPins, ROWS, COLS);

    // Initialize lastStates for all logical buttons
    buttonMatrix->getKeys();
    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            uint8_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            bool physicalPressed = buttonMatrix->isPressed(keyChar);
            
            // Initialize all logical buttons at this position
            for (uint8_t btnIdx = 0; btnIdx < logicalButtonCounts[idx]; btnIdx++) {
                bool effectivePressed = physicalPressed;
                if (logicalButtons[idx][btnIdx].reverse) {
                    effectivePressed = !effectivePressed;
                }
                logicalButtons[idx][btnIdx].lastState = effectivePressed;
                logicalButtons[idx][btnIdx].lastMomentaryState = effectivePressed;
            }
        }
    }
}

void updateMatrix() {
    // Add a small delay to prevent rapid scanning that could cause double-triggers
    static unsigned long lastScanTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastScanTime < 5) {  // Minimum 5ms between scans
        return;
    }
    lastScanTime = currentTime;
    
    // First, do the regular button matrix scanning
    if (buttonMatrix->getKeys()) {
        // Process key events
        for (int i = 0; i < MATRIX_MAX_KEYS; i++) {
            if (buttonMatrix->key[i].stateChanged) {
                char keyChar = buttonMatrix->key[i].kchar;
                MatrixKeyState keyState = buttonMatrix->key[i].kstate;
                
                // Find the index for this key
                uint8_t idx = 0;
                for (uint8_t j = 0; j < ROWS * COLS; j++) {
                    if (keymap[j] == keyChar) {
                        idx = j;
                        break;
                    }
                }
                
                // Process all logical buttons at this position
                for (uint8_t btnIdx = 0; btnIdx < logicalButtonCounts[idx]; btnIdx++) {
                    MatrixLogicalButton& logicalBtn = logicalButtons[idx][btnIdx];
                    
                    // Skip encoder behaviors in matrix (they should be handled by EncoderInput)
                    if (logicalBtn.behavior == ENC_A || logicalBtn.behavior == ENC_B) continue;

                    // Apply reverse logic to the key states
                    MatrixKeyState effectiveKeyState = keyState;
                    if (logicalBtn.reverse) {
                        switch (keyState) {
                            case MATRIX_PRESSED:
                                effectiveKeyState = MATRIX_RELEASED;
                                break;
                            case MATRIX_RELEASED:
                                effectiveKeyState = MATRIX_PRESSED;
                                break;
                            case MATRIX_HELD:
                                effectiveKeyState = MATRIX_IDLE; // When reversed, held becomes idle
                                break;
                            case MATRIX_IDLE:
                                effectiveKeyState = MATRIX_HELD; // When reversed, idle becomes held
                                break;
                        }
                    }

                    uint8_t joyIdx = (logicalBtn.joyButtonID > 0) ? (logicalBtn.joyButtonID - 1) : 0;

                    switch (logicalBtn.behavior) {
                        case NORMAL:
                            if (effectiveKeyState == MATRIX_PRESSED) {
                                MyJoystick.setButton(joyIdx, 1);  // Button pressed
                            } else if (effectiveKeyState == MATRIX_RELEASED) {
                                MyJoystick.setButton(joyIdx, 0);  // Button released
                            }
                            // Don't send events for MATRIX_HELD state
                            break;
                        case MOMENTARY:
                            if (effectiveKeyState == MATRIX_PRESSED && !logicalBtn.lastMomentaryState) {
                                MyJoystick.setButton(joyIdx, 1);
                                delay(10);
                                MyJoystick.setButton(joyIdx, 0);
                            }
                            logicalBtn.lastMomentaryState = (effectiveKeyState == MATRIX_PRESSED || effectiveKeyState == MATRIX_HELD);
                            break;
                        case ENC_A:
                        case ENC_B:
                            break;
                    }
                    logicalBtn.lastState = (effectiveKeyState == MATRIX_PRESSED || effectiveKeyState == MATRIX_HELD);
                }
            }
        }
    }
    
    // Update encoder pin states based on current matrix scan results
    // Initialize all encoder pin states to HIGH (default pullup state)
    for (uint8_t pin = 0; pin < 20; pin++) {
        g_encoderMatrixPinStates[pin] = 1;
    }
    
    // Update encoder pin states based on current button states
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            uint8_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            bool isPressed = buttonMatrix->isPressed(keyChar);
            
            // If any button in this row is pressed, mark the row pin as LOW
            if (isPressed) {
                g_encoderMatrixPinStates[rowPins[r]] = 0;
            }
        }
    }
}