#include "MatrixInput.h"
#include "Config.h"
#include "JoystickWrapper.h"
#include "Keypad.h"

// Dynamic matrix config
static uint8_t ROWS = 0;
static uint8_t COLS = 0;
static byte* rowPins = nullptr;
static byte* colPins = nullptr;
static char* keymap = nullptr;  // Changed from char** to char*
static Keypad* keypad = nullptr;
static uint8_t* joyButtonIDs = nullptr;
static ButtonBehavior* behaviors = nullptr;
static bool* lastStates = nullptr;
static bool* lastMomentaryStates = nullptr;

bool g_encoderMatrixPinStates[20] = {1}; // indexed by pin number, default HIGH

void initMatrixFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    // Find max row/col and count matrix buttons
    uint8_t maxRow = 0, maxCol = 0, count = 0;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == LOGICAL_MATRIX) {
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

    // Fill row/col pins from hardwarePins, but exclude encoder pins
    uint8_t rowIdx = 0, colIdx = 0;
    for (uint8_t pin = 0; pin < hardwarePinsCount; ++pin) {
        // Check if this pin is used by an encoder
        bool isEncoderPin = false;
        for (uint8_t i = 0; i < logicalCount; ++i) {
            if (logicals[i].type == LOGICAL_BTN && 
                (logicals[i].u.btn.behavior == ENC_A || logicals[i].u.btn.behavior == ENC_B) &&
                logicals[i].u.btn.pin == pin) {
                isEncoderPin = true;
                break;
            }
        }
        
        // Only add to matrix if not used by encoder
        if (!isEncoderPin) {
            if (hardwarePins[pin] == BTN_ROW && rowIdx < ROWS) rowPins[rowIdx++] = pin;
            if (hardwarePins[pin] == BTN_COL && colIdx < COLS) colPins[colIdx++] = pin;
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
        if (logicals[i].type == LOGICAL_MATRIX) {
            uint8_t idx = logicals[i].u.matrix.row * COLS + logicals[i].u.matrix.col;
            joyButtonIDs[idx] = logicals[i].u.matrix.joyButtonID;
            behaviors[idx] = logicals[i].u.matrix.behavior;
        }
    }

    // Create keypad - pass the flat keymap directly
    //if (keypad) delete keypad;
    keypad = new Keypad(keymap, rowPins, colPins, ROWS, COLS);

    // Initialize lastStates to current state (like ButtonInput.cpp does)
    keypad->getKeys();
    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            uint8_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            lastStates[idx] = keypad->isPressed(keyChar);
        }
    }
}

void updateMatrix() {
    // First, do the regular keypad scanning
    if (keypad->getKeys()) {
        // Process key events
        for (int i = 0; i < LIST_MAX; i++) {
            if (keypad->key[i].stateChanged) {
                char keyChar = keypad->key[i].kchar;
                bool pressed = (keypad->key[i].kstate == PRESSED || keypad->key[i].kstate == HOLD);
                
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

                // Skip encoder behaviors in matrix (they should be handled by EncoderInput)
                if (behavior == ENC_A || behavior == ENC_B) continue;

                switch (behavior) {
                    case NORMAL:
                        Joystick.setButton(joyButtonIDs[idx], pressed);
                        break;
                    case MOMENTARY:
                        if (pressed && !lastMomentaryStates[idx]) {
                            Joystick.setButton(joyButtonIDs[idx], 1);
                            delay(10);
                            Joystick.setButton(joyButtonIDs[idx], 0);
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
    // This scans each column independently to get the true state of each row pin
    for (uint8_t c = 0; c < COLS; c++) {
        // Set this column LOW
        pinMode(colPins[c], OUTPUT);
        digitalWrite(colPins[c], LOW);
        
        // Set all other columns HIGH
        for (uint8_t otherC = 0; otherC < COLS; otherC++) {
            if (otherC != c) {
                pinMode(colPins[otherC], OUTPUT);
                digitalWrite(colPins[otherC], HIGH);
            }
        }
        
        // Small delay for pin states to stabilize
        delayMicroseconds(10);
        
        // Now read all row pins - they will be LOW if connected to the active column
        for (uint8_t r = 0; r < ROWS; r++) {
            pinMode(rowPins[r], INPUT_PULLUP);
            // Update the encoder pin state for this row pin
            g_encoderMatrixPinStates[rowPins[r]] = digitalRead(rowPins[r]);
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