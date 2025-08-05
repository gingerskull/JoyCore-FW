// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonMatrix.h"

ButtonMatrix::ButtonMatrix(char* keymap, byte* rowPins, byte* colPins, uint8_t numRows, uint8_t numCols)
    : keymap(keymap), rowPins(rowPins), colPins(colPins), numRows(numRows), numCols(numCols), debounceTime(20) {
    
    uint8_t totalKeys = numRows * numCols;
    
    // Allocate memory for state tracking
    currentStates = new bool[totalKeys]();
    lastStates = new bool[totalKeys]();
    lastChangeTime = new unsigned long[totalKeys]();
    
    // Initialize key array
    for (uint8_t i = 0; i < MATRIX_MAX_KEYS; i++) {
        key[i].kchar = 0;
        key[i].kstate = MATRIX_IDLE;
        key[i].stateChanged = false;
    }
    
    // Configure pins
    for (uint8_t i = 0; i < numRows; i++) {
        pinMode(rowPins[i], INPUT_PULLUP);
    }
    for (uint8_t i = 0; i < numCols; i++) {
        pinMode(colPins[i], INPUT_PULLUP);
    }
    
    // Initialize last change times
    unsigned long currentTime = millis();
    for (uint8_t i = 0; i < totalKeys; i++) {
        lastChangeTime[i] = currentTime;
    }
}

ButtonMatrix::~ButtonMatrix() {
    delete[] currentStates;
    delete[] lastStates;
    delete[] lastChangeTime;
}

void ButtonMatrix::scanMatrix() {
    unsigned long currentTime = millis();
    uint8_t totalKeys = numRows * numCols;
    
    // Clear all state change flags
    for (uint8_t i = 0; i < MATRIX_MAX_KEYS && i < totalKeys; i++) {
        key[i].stateChanged = false;
    }
    
    // Scan each column
    for (uint8_t col = 0; col < numCols; col++) {
        // Set current column as output LOW
        pinMode(colPins[col], OUTPUT);
        digitalWrite(colPins[col], LOW);
        
        // Set all other columns as INPUT_PULLUP (high impedance)
        for (uint8_t otherCol = 0; otherCol < numCols; otherCol++) {
            if (otherCol != col) {
                pinMode(colPins[otherCol], INPUT_PULLUP);
            }
        }
        
        // Small delay for pin states to stabilize
        delayMicroseconds(10);
        
        // Read all row pins
        for (uint8_t row = 0; row < numRows; row++) {
            uint8_t keyIndex = row * numCols + col;
            bool pinState = digitalRead(rowPins[row]);
            bool pressed = (pinState == LOW); // Button pressed when pin is pulled LOW
            
            // Check if state changed and debounce time has passed
            if (pressed != lastStates[keyIndex] && (currentTime - lastChangeTime[keyIndex]) >= debounceTime) {
                currentStates[keyIndex] = pressed;
                lastChangeTime[keyIndex] = currentTime;
                
                // Update key structure (compatible with Keypad library)
                if (keyIndex < MATRIX_MAX_KEYS) {
                    key[keyIndex].kchar = keymap[keyIndex];
                    key[keyIndex].stateChanged = true;
                    
                    if (pressed) {
                        if (lastStates[keyIndex] == false) {
                            key[keyIndex].kstate = MATRIX_PRESSED;
                        } else {
                            key[keyIndex].kstate = MATRIX_HELD;
                        }
                    } else {
                        key[keyIndex].kstate = MATRIX_RELEASED;
                    }
                }
                
                lastStates[keyIndex] = pressed;
            } else {
                // No state change, but update current state for consistency
                currentStates[keyIndex] = pressed;
                
                // Update held state if button is still pressed
                if (keyIndex < MATRIX_MAX_KEYS && pressed && lastStates[keyIndex]) {
                    key[keyIndex].kchar = keymap[keyIndex];
                    key[keyIndex].kstate = MATRIX_HELD;
                    // Don't set stateChanged for held state
                }
            }
        }
    }
    
    // Restore all pins to INPUT_PULLUP state
    for (uint8_t row = 0; row < numRows; row++) {
        pinMode(rowPins[row], INPUT_PULLUP);
    }
    for (uint8_t col = 0; col < numCols; col++) {
        pinMode(colPins[col], INPUT_PULLUP);
    }
}

bool ButtonMatrix::getKeys() {
    scanMatrix();
    
    // Check if any key state changed
    uint8_t totalKeys = numRows * numCols;
    for (uint8_t i = 0; i < MATRIX_MAX_KEYS && i < totalKeys; i++) {
        if (key[i].stateChanged) {
            return true;
        }
    }
    return false;
}

bool ButtonMatrix::isPressed(char keyChar) {
    uint8_t totalKeys = numRows * numCols;
    
    // Find the key index for the given character
    for (uint8_t i = 0; i < totalKeys; i++) {
        if (keymap[i] == keyChar) {
            return currentStates[i];
        }
    }
    return false;
}

void ButtonMatrix::setDebounceTime(uint8_t debounce) {
    debounceTime = debounce;
}
