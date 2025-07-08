#pragma once
#include <Arduino.h>
#include "Config.h"

void initMatrix();
void initMatrixFromLogical(const LogicalInput* logicals, uint8_t logicalCount);
void updateMatrix();
