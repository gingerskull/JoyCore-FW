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
static uint8_t* joyButtonIDs = nullptr;
static ButtonBehavior* behaviors = nullptr;
static bool* lastStates = nullptr;
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
    if (keymap) delete[] keymap;  // Changed from keys cleanup
    if (joyButtonIDs) delete[] joyButtonIDs;
    if (behaviors) delete[] behaviors;
    if (lastStates) delete[] lastStates;
    if (lastMomentaryStates) delete[] lastMomentaryStates;

    rowPins = new byte[ROWS];
    colPins = new byte[COLS];
    keymap = new char[ROWS * COLS];  // Flat array for keymap
    joyButtonIDs = new uint8_t[ROWS * COLS];
    behaviors = new ButtonBehavior[ROWS * COLS];
    lastStates = new bool[ROWS * COLS]{}; // All false (not pressed)
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

    // Fill keymap, joyButtonIDs, behaviors
    for (uint8_t r = 0; r < ROWS; ++r)
        for (uint8_t c = 0; c < COLS; ++c)
            keymap[r * COLS + c] = 'A' + r * COLS + c; // unique char

    for (uint8_t i = 0; i < ROWS * COLS; ++i) {
        joyButtonIDs[i] = 0xFF; // default invalid
        behaviors[i] = NORMAL;
    }
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t idx = logicals[i].u.matrix.row * COLS + logicals[i].u.matrix.col;
            joyButtonIDs[idx] = logicals[i].u.matrix.joyButtonID;
            behaviors[idx] = logicals[i].u.matrix.behavior;
        }
    }

    // Create button matrix - pass the flat keymap directly
    buttonMatrix = new ButtonMatrix(keymap, rowPins, colPins, ROWS, COLS);

    // Initialize lastStates to current state (like ButtonInput.cpp does)
    buttonMatrix->getKeys();
    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            uint8_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            lastStates[idx] = buttonMatrix->isPressed(keyChar);
        }
    }
}

void updateMatrix() {
    // First, do the regular button matrix scanning
    if (buttonMatrix->getKeys()) {
        // Process key events
        for (int i = 0; i < MATRIX_MAX_KEYS; i++) {
            if (buttonMatrix->key[i].stateChanged) {
                char keyChar = buttonMatrix->key[i].kchar;
                bool pressed = (buttonMatrix->key[i].kstate == MATRIX_PRESSED || buttonMatrix->key[i].kstate == MATRIX_HELD);
                
                // Find the index for this key
                uint8_t idx = 0;
                for (uint8_t j = 0; j < ROWS * COLS; j++) {
                    if (keymap[j] == keyChar) {
                        idx = j;
                        break;
                    }
                }
                
                if (joyButtonIDs[idx] == 0xFF) continue; // skip unused
                ButtonBehavior behavior = behaviors[idx];

                uint8_t joyIdx = (joyButtonIDs[idx] > 0) ? (joyButtonIDs[idx] - 1) : 0;
                // Skip encoder behaviors in matrix (they should be handled by EncoderInput)
                if (behavior == ENC_A || behavior == ENC_B) continue;

                switch (behavior) {
                    case NORMAL:
                        MyJoystick.setButton(joyIdx, pressed);
                        break;
                    case MOMENTARY:
                        if (pressed && !lastMomentaryStates[idx]) {
                            MyJoystick.setButton(joyIdx, 1);
                            delay(10);
                            MyJoystick.setButton(joyIdx, 0);
                        }
                        lastMomentaryStates[idx] = pressed;
                        break;
                    case ENC_A:
                    case ENC_B:
                        break;
                }
                lastStates[idx] = pressed;
            }
        }
    }
    
    // Now do a separate scan specifically for encoder pin states
    // Initialize all encoder pin states to HIGH (default pullup state)
    for (uint8_t pin = 0; pin < 20; pin++) {
        g_encoderMatrixPinStates[pin] = 1;
    }
    
    // Scan each column independently to get the true state of each row pin
    for (uint8_t c = 0; c < COLS; c++) {
        // Set this column LOW
        pinMode(colPins[c], OUTPUT);
        digitalWrite(colPins[c], LOW);
        
        // Set all other columns as INPUT_PULLUP (floating high)
        for (uint8_t otherC = 0; otherC < COLS; otherC++) {
            if (otherC != c) {
                pinMode(colPins[otherC], INPUT_PULLUP);
            }
        }
        
        // Small delay for pin states to stabilize
        delayMicroseconds(10);
        
        // Now read all row pins - they will be LOW if connected to the active column
        for (uint8_t r = 0; r < ROWS; r++) {
            pinMode(rowPins[r], INPUT_PULLUP);
            bool pinState = digitalRead(rowPins[r]);
            
            // Only update to LOW if we detect a LOW state
            // This preserves the state across multiple column scans
            if (pinState == LOW) {
                g_encoderMatrixPinStates[rowPins[r]] = 0;
            }
        }
    }
    
    // Restore all pins to their default state
    for (uint8_t r = 0; r < ROWS; r++) {
        pinMode(rowPins[r], INPUT_PULLUP);
    }
    for (uint8_t c = 0; c < COLS; c++) {
        pinMode(colPins[c], INPUT_PULLUP);
    }
}