// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "../../Config.h"

void initMatrix();
void initMatrixFromLogical(const LogicalInput* logicals, uint8_t logicalCount);
void updateMatrix();

// Optional: allocation summary for debug
uint8_t getMatrixRows();
uint8_t getMatrixCols();

// Raw state access for configuration tools
namespace MatrixRawAccess {
    uint8_t* getRowPins();
    uint8_t* getColPins();
}
