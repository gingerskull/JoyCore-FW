// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "Config.h"

/**
 * @brief Configuration for a single button input
 */
struct ButtonConfig {
    uint8_t pin;
    uint8_t joyButtonID;
    ButtonBehavior behavior;
    uint8_t reverse;
};

/**
 * @brief Initialize buttons with configuration array
 * @param configs Array of button configurations
 * @param count Number of buttons to initialize
 */
void initButtons(const ButtonConfig* configs, uint8_t count);

/**
 * @brief Initialize buttons from logical input configuration
 * @param logicals Array of logical input definitions
 * @param logicalCount Number of logical inputs
 */
void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount);

/**
 * @brief Update button states and send joystick events
 */
void updateButtons();

// Helper functions for button initialization
void updateShiftRegisterButtons();
bool isRegularButton(const LogicalInput& input);
void initRegularButtons(const LogicalInput* logicals, uint8_t logicalCount, uint8_t count);
void initShiftRegisterIfNeeded(const LogicalInput* logicals, uint8_t logicalCount); 