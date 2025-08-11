// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// Button matrix scanner - replacement for external Keypad library
// Provides simple matrix button scanning with state change detection
//
// This implementation uses dynamic allocation sized to the configured
// matrix (numRows * numCols) to minimize memory usage.

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

    // Dynamic storage sized to totalKeys = numRows * numCols
    bool* currentStates;   // Current button states
    bool* lastStates;      // Previous button states
    unsigned long* lastChangeTime; // Last change time for each key
    uint16_t totalKeys;    // Cached total keys

    uint8_t debounceTime;  // Debounce delay in milliseconds
    
    void scanMatrix();    // Internal matrix scanning function
    
public:
    // Array of key states (compatible with Keypad library). Length is keyCount.
    MatrixKey* key;
    uint16_t keyCount;
    
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
    
    // Accessors for dynamic key array
    inline uint16_t getKeyCount() const { return keyCount; }
    inline const MatrixKey* getKeyArray() const { return key; }
};
