// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// Button matrix scanner - replacement for external Keypad library
// Provides simple matrix button scanning with state change detection
// 
// This is a built-in implementation that replaces the Chris--A/Keypad library
// dependency while maintaining API compatibility with the existing MatrixInput.cpp code.
// Features:
// - Row/column matrix scanning
// - Debounce support
// - State change detection
// - Compatible key structure format

#define MATRIX_MAX_KEYS 32  // Maximum number of keys that can be tracked

enum MatrixKeyState : uint8_t {
    MATRIX_IDLE = 0,
    MATRIX_PRESSED = 1,
    MATRIX_HELD = 2,
    MATRIX_RELEASED = 3
};

struct MatrixKey {
    char kchar;           // Character representing this key
    MatrixKeyState kstate; // Current state
    bool stateChanged;    // True if state changed since last scan
};

class ButtonMatrix {
private:
    char* keymap;         // Flat array of key characters
    byte* rowPins;        // Array of row pins
    byte* colPins;        // Array of column pins
    uint8_t numRows;      // Number of rows
    uint8_t numCols;      // Number of columns
    
    bool* currentStates;  // Current button states
    bool* lastStates;     // Previous button states
    
    uint8_t debounceTime; // Debounce delay in milliseconds
    unsigned long* lastChangeTime; // Last change time for each key
    
    void scanMatrix();    // Internal matrix scanning function
    
public:
    MatrixKey key[MATRIX_MAX_KEYS]; // Array of key states (compatible with Keypad library)
    
    // Constructor
    ButtonMatrix(char* keymap, byte* rowPins, byte* colPins, uint8_t numRows, uint8_t numCols);
    
    // Destructor
    ~ButtonMatrix();
    
    // Scan the matrix and update key states
    // Returns true if any key state changed
    bool getKeys();
    
    // Check if a specific key is currently pressed
    bool isPressed(char keyChar);
    
    // Set debounce time (default is 10ms)
    void setDebounceTime(uint8_t debounce);
};
