#pragma once
#include <Arduino.h>
#include "Config.h"

// Initializes all buttons using the config array
void initButtons(const ButtonConfig* configs, uint8_t count);

// Updates button states and sends HID events
void updateButtons();
