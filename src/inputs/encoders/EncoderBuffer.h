// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// Timing constants for encoder buffer system
static const uint32_t PRESS_INTERVAL_US = 40000;  // 40ms interval between presses
static const uint32_t PRESS_DURATION_US = 40000;  // 40ms press duration for USB

/**
 * @brief Buffer structure for managing encoder timing and USB output
 */
struct EncoderBuffer {
    uint8_t cwButtonId;
    uint8_t ccwButtonId;
    uint8_t pendingCwSteps;
    uint8_t pendingCcwSteps;
    uint32_t lastUsbPressTime;  // Timing for USB output
    bool usbButtonPressed;      // State for USB output
    uint8_t currentDirection;   // 0 = none, 1 = CW, 2 = CCW
};

/**
 * @brief Initialize encoder buffer system
 */
// Initialize encoder buffer system, optionally reserving capacity
void initEncoderBuffers(uint8_t capacity = 0);

/**
 * @brief Add steps to buffer for consistent timing
 * @param buttonId The button ID to add steps for
 * @param steps Number of steps to add
 */
void addEncoderSteps(uint8_t buttonId, uint8_t steps);

/**
 * @brief Process timing buffers for consistent intervals
 */
void processEncoderBuffers();

/**
 * @brief Set up buffer entry for an encoder pair
 * @param cwButtonId Clockwise button ID
 * @param ccwButtonId Counter-clockwise button ID
 * @return Index of the created buffer entry, or 255 if failed
 */
uint8_t createEncoderBufferEntry(uint8_t cwButtonId, uint8_t ccwButtonId);

/**
 * @brief Get the current buffer count
 * @return Number of active encoder buffers
 */
uint8_t getEncoderBufferCount(); 