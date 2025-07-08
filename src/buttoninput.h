#pragma once
#include <Arduino.h>
#include "Config.h"

// Define ButtonConfig locally for button input handling
struct ButtonConfig {
    uint8_t pin;
    uint8_t joyButtonID;
    ButtonBehavior behavior;
};

// Initializes all buttons using the config array
void initButtons(const ButtonConfig* configs, uint8_t count);
void initButtonsFromLogical(const LogicalInput* logicals, uint8_t logicalCount);

// Updates button states and sends HID events
void updateButtons();
