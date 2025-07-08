#include "MatrixInput.h"
#include "Config.h"
#include "JoystickWrapper.h"
#include "Keypad.h"

// Dynamic matrix config
static uint8_t ROWS = 0;
static uint8_t COLS = 0;
static byte* rowPins = nullptr;
static byte* colPins = nullptr;
static char** keys = nullptr;
static Keypad* keypad = nullptr;
static uint8_t* joyButtonIDs = nullptr;
static ButtonBehavior* behaviors = nullptr;
static bool* lastStates = nullptr;
static bool* lastMomentaryStates = nullptr;

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
    if (keys) {
        for (uint8_t i = 0; i < ROWS; ++i) delete[] keys[i];
        delete[] keys;
    }
    if (joyButtonIDs) delete[] joyButtonIDs;
    if (behaviors) delete[] behaviors;
    if (lastStates) delete[] lastStates;
    if (lastMomentaryStates) delete[] lastMomentaryStates;

    rowPins = new byte[ROWS];
    colPins = new byte[COLS];
    keys = new char*[ROWS];
    for (uint8_t i = 0; i < ROWS; ++i) keys[i] = new char[COLS];
    joyButtonIDs = new uint8_t[ROWS * COLS];
    behaviors = new ButtonBehavior[ROWS * COLS];
    lastStates = new bool[ROWS * COLS]{};
    lastMomentaryStates = new bool[ROWS * COLS]{};

    // Fill row/col pins from hardwarePins
    uint8_t rowIdx = 0, colIdx = 0;
    for (uint8_t pin = 0; pin < sizeof(hardwarePins)/sizeof(hardwarePins[0]); ++pin) {
        if (hardwarePins[pin] == BTN_ROW && rowIdx < ROWS) rowPins[rowIdx++] = pin;
        if (hardwarePins[pin] == BTN_COL && colIdx < COLS) colPins[colIdx++] = pin;
    }

    // Fill keymap, joyButtonIDs, behaviors
    for (uint8_t r = 0; r < ROWS; ++r)
        for (uint8_t c = 0; c < COLS; ++c)
            keys[r][c] = 'A' + r * COLS + c; // unique char

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

    // Create keypad
    if (keypad) delete keypad;
    keypad = new Keypad(makeKeymap(*keys), rowPins, colPins, ROWS, COLS);
}

void updateMatrix() {
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t cidx = 0; cidx < COLS; cidx++) {
            uint8_t idx = r * COLS + cidx;
            if (joyButtonIDs[idx] == 0xFF) continue; // skip unused
            char keyChar = keys[r][cidx];
            bool pressed = keypad->isPressed(keyChar);
            ButtonBehavior behavior = behaviors[idx];

            switch (behavior) {
                case NORMAL:
                    if (pressed != lastStates[idx]) {
                        Joystick.setButton(joyButtonIDs[idx], pressed);
                        lastStates[idx] = pressed;
                    }
                    break;
                case MOMENTARY:
                    if (!lastMomentaryStates[idx] && pressed) {
                        Joystick.setButton(joyButtonIDs[idx], 1);
                        delay(10);
                        Joystick.setButton(joyButtonIDs[idx], 0);
                    }
                    lastMomentaryStates[idx] = pressed;
                    break;
            }
        }
    }
    keypad->getKeys(); // Update keypad state
}
