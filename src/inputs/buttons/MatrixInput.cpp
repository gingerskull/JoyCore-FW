// SPDX-License-Identifier: GPL-3.0-or-later
#include "MatrixInput.h"
#include "../../Config.h"
#include "../../rp2040/JoystickWrapper.h"
#include "ButtonMatrix.h"
#include <new>
#include "../PoolConfig.h"
#include "LogicalButton.h"

// Static matrix config and storage
static uint8_t ROWS = 0;
static uint8_t COLS = 0;
static byte rowPins[MAX_MATRIX_ROWS];
static byte colPins[MAX_MATRIX_COLS];
static char keymap[MAX_MATRIX_ROWS * MAX_MATRIX_COLS];
// ButtonMatrix instance (placement-new to avoid heap)
static ButtonMatrix* buttonMatrix = nullptr;
static uint8_t buttonMatrixStorage[sizeof(ButtonMatrix)];

// Per-position logical button storage
static RuntimeLogicalButton matrixLogicalButtons[MAX_MATRIX_ROWS * MAX_MATRIX_COLS][MAX_LOGICAL_PER_MATRIX_POS];
static uint8_t matrixLogicalCounts[MAX_MATRIX_ROWS * MAX_MATRIX_COLS];

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
    if (ROWS > MAX_MATRIX_ROWS) ROWS = MAX_MATRIX_ROWS;
    if (COLS > MAX_MATRIX_COLS) COLS = MAX_MATRIX_COLS;
    uint16_t total = ROWS * COLS;
    for (uint16_t i = 0; i < total; ++i) matrixLogicalCounts[i] = 0;

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
            if (type == BTN_ROW && rowIdx < ROWS) rowPins[rowIdx++] = atoi(pinName);
            if (type == BTN_COL && colIdx < COLS) colPins[colIdx++] = atoi(pinName);
        }
    }

    for (uint8_t r = 0; r < ROWS; ++r) {
        for (uint8_t c = 0; c < COLS; ++c) {
            keymap[r * COLS + c] = 'A' + r * COLS + c;
        }
    }

    // Fill logical buttons
    for (uint8_t i = 0; i < logicalCount; ++i) {
        if (logicals[i].type == INPUT_MATRIX) {
            uint8_t r = logicals[i].u.matrix.row;
            uint8_t c = logicals[i].u.matrix.col;
            if (r < ROWS && c < COLS) {
                uint16_t idx = r * COLS + c;
                uint8_t &cnt = matrixLogicalCounts[idx];
                if (cnt < MAX_LOGICAL_PER_MATRIX_POS) {
                    RuntimeLogicalButton &dest = matrixLogicalButtons[idx][cnt++];
                    dest.joyButtonID = logicals[i].u.matrix.joyButtonID;
                    dest.behavior = logicals[i].u.matrix.behavior;
                    dest.reverse = logicals[i].u.matrix.reverse;
                    dest.lastState = false;
                    dest.momentaryStartTime = 0;
                    dest.momentaryActive = false;
                }
            }
        }
    }

    if (!buttonMatrix) {
        buttonMatrix = new (buttonMatrixStorage) ButtonMatrix(keymap, rowPins, colPins, ROWS, COLS);
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
        for (int i = 0; i < MATRIX_MAX_KEYS; i++) {
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