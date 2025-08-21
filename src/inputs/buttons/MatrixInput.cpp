// SPDX-License-Identifier: GPL-3.0-or-later
#include "MatrixInput.h"
#include "../../Config.h"
#include "../../rp2040/JoystickWrapper.h"
#include "ButtonMatrix.h"
#include <new>
#include "LogicalButton.h"

// Dynamic matrix config and storage
static uint8_t ROWS = 0;
static uint8_t COLS = 0;
static uint16_t PREV_TOTAL = 0;
static byte* rowPins = nullptr;
static byte* colPins = nullptr;
static char* keymap = nullptr;
// ButtonMatrix instance
static ButtonMatrix* buttonMatrix = nullptr;

// Per-position logical button storage
static RuntimeLogicalButton** matrixLogicalButtons = nullptr; // [ROWS*COLS][variable]
static uint8_t* matrixLogicalCounts = nullptr; // counts per position

bool g_encoderMatrixPinStates[20] = {1};

static bool pinEqualsName(uint8_t pin, const char* pinName) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", pin);
    return strcmp(buf, pinName) == 0;
}

void initMatrixFromLogical(const LogicalInput* logicals, uint8_t logicalCount) {
    uint8_t maxRow = 0, maxCol = 0;
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            if (logicals[i].u.matrix.row > maxRow) maxRow = logicals[i].u.matrix.row;
            if (logicals[i].u.matrix.col > maxCol) maxCol = logicals[i].u.matrix.col;
        }
    }
    ROWS = maxRow + 1;
    COLS = maxCol + 1;
    // Allocate dynamic storage sized to discovered matrix
    uint16_t total = (uint16_t)ROWS * (uint16_t)COLS;
    // Free previous allocations if reinitialized
    delete[] rowPins; rowPins = nullptr;
    delete[] colPins; colPins = nullptr;
    delete[] keymap; keymap = nullptr;
    if (matrixLogicalButtons) {
        for (uint16_t i = 0; i < PREV_TOTAL; ++i) delete[] matrixLogicalButtons[i];
        delete[] matrixLogicalButtons;
        matrixLogicalButtons = nullptr;
    }
    delete[] matrixLogicalCounts; matrixLogicalCounts = nullptr;

    if (ROWS == 0 || COLS == 0) return;

    rowPins = new byte[ROWS]();
    colPins = new byte[COLS]();
    keymap = new char[total]();
    matrixLogicalCounts = new uint8_t[total]();
    matrixLogicalButtons = new RuntimeLogicalButton*[total]();
    PREV_TOTAL = total;

    // Fill row/col pins excluding encoder pins
    uint8_t rowIdx = 0, colIdx = 0;
    for (uint8_t i = 0; i < hardwarePinMapCount; ++i) {
        HardwarePinName pinName = hardwarePinMap[i].name;
        bool isEncoderPin = false;
        for (uint8_t j = 0; j < logicalCount; ++j) {
            if (logicals[j].type == INPUT_PIN &&
                (logicals[j].u.pin.behavior == ENC_A || logicals[j].u.pin.behavior == ENC_B) &&
                pinEqualsName(logicals[j].u.pin.pin, pinName)) { isEncoderPin = true; break; }
        }
        if (!isEncoderPin) {
            PinType type = getPinType(pinName);
            if (type == BTN_ROW && rowIdx < ROWS) rowPins[rowIdx++] = (byte)atoi(pinName);
            if (type == BTN_COL && colIdx < COLS) colPins[colIdx++] = (byte)atoi(pinName);
        }
    }

    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            keymap[r * COLS + c] = 'A' + r * COLS + c;
        }
    }

    // Fill logical buttons (per position dynamic arrays sized to count)
    // First pass: count per position
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t r = logicals[i].u.matrix.row;
            uint8_t c = logicals[i].u.matrix.col;
            if (r < ROWS && c < COLS) {
                uint16_t idx = r * COLS + c;
                matrixLogicalCounts[idx]++;
            }
        }
    }
    // Allocate per position arrays
    for (uint16_t idx = 0; idx < total; ++idx) {
        uint8_t cnt = matrixLogicalCounts[idx];
        matrixLogicalButtons[idx] = (cnt > 0) ? new RuntimeLogicalButton[cnt]() : nullptr;
        matrixLogicalCounts[idx] = 0; // reuse as write index in next pass
    }
    // Second pass: populate
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t r = logicals[i].u.matrix.row;
            uint8_t c = logicals[i].u.matrix.col;
            if (r < ROWS && c < COLS) {
                uint16_t idx = r * COLS + c;
                uint8_t w = matrixLogicalCounts[idx]++;
                RuntimeLogicalButton &dest = matrixLogicalButtons[idx][w];
                dest.joyButtonID = logicals[i].u.matrix.joyButtonID;
                dest.behavior = logicals[i].u.matrix.behavior;
                dest.reverse = logicals[i].u.matrix.reverse;
                dest.lastState = false;
                dest.momentaryStartTime = 0;
                dest.momentaryActive = false;
            }
        }
    }

    if (!buttonMatrix) {
        buttonMatrix = new ButtonMatrix(keymap, rowPins, colPins, ROWS, COLS);
    } else {
        // Recreate on size change
        delete buttonMatrix;
        buttonMatrix = new ButtonMatrix(keymap, rowPins, colPins, ROWS, COLS);
    }

    buttonMatrix->getKeys();
    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            uint16_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            bool physicalPressed = buttonMatrix->isPressed(keyChar);
            for (uint8_t b = 0; b < matrixLogicalCounts[idx]; ++b) {
                RuntimeLogicalButton &btn = matrixLogicalButtons[idx][b];
                bool effective = physicalPressed;
                if (btn.reverse) effective = !effective;
                btn.lastState = effective;
                btn.momentaryActive = false;
            }
        }
    }
}

void updateMatrix() {
    uint32_t now = millis();
    if (buttonMatrix->getKeys()) {
    for (uint16_t i = 0; i < buttonMatrix->getKeyCount(); i++) {
            if (buttonMatrix->key[i].stateChanged) {
                char keyChar = buttonMatrix->key[i].kchar;
                MatrixKeyState keyState = buttonMatrix->key[i].kstate;
                uint16_t idx = 0;
                for (uint16_t j = 0; j < (uint16_t)ROWS * COLS; ++j) { if (keymap[j] == keyChar) { idx = j; break; } }
        for (uint8_t b = 0; b < matrixLogicalCounts[idx]; ++b) {
                    RuntimeLogicalButton &btn = matrixLogicalButtons[idx][b];
                    if (btn.behavior == ENC_A || btn.behavior == ENC_B) continue;
                    bool physicalPressed = (keyState == MATRIX_PRESSED || keyState == MATRIX_HELD);
                    processLogicalButton(now, physicalPressed, btn);
                }
            }
        }
    }

    for (uint8_t pin = 0; pin < 20; pin++) g_encoderMatrixPinStates[pin] = 1;
    for (uint8_t r = 0; r < ROWS; r++) {
        for (uint8_t c = 0; c < COLS; c++) {
            uint16_t idx = r * COLS + c;
            char keyChar = keymap[idx];
            bool isPressed = buttonMatrix->isPressed(keyChar);
            if (isPressed) g_encoderMatrixPinStates[rowPins[r]] = 0;
        }
    }
}

uint8_t getMatrixRows() { return ROWS; }
uint8_t getMatrixCols() { return COLS; }

// Raw state access for configuration tools
namespace MatrixRawAccess {
    uint8_t* getRowPins() { return rowPins; }
    uint8_t* getColPins() { return colPins; }
}