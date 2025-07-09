// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "Config.h"

void initShiftRegEncoders(const LogicalInput* logicals, uint8_t logicalCount);
void updateShiftRegEncoders(uint8_t* shiftRegBuffer);
