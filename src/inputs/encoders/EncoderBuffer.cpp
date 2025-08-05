// SPDX-License-Identifier: GPL-3.0-or-later
#include "EncoderBuffer.h"
#include "../../rp2040/JoystickWrapper.h"

// Global encoder buffer storage
static EncoderBuffer encoderBuffers[MAX_ENCODERS];
static uint8_t bufferCount = 0;

void initEncoderBuffers() {
    bufferCount = 0;
    for (uint8_t i = 0; i < MAX_ENCODERS; i++) {
        encoderBuffers[i].cwButtonId = 0;
        encoderBuffers[i].ccwButtonId = 0;
        encoderBuffers[i].pendingCwSteps = 0;
        encoderBuffers[i].pendingCcwSteps = 0;
        encoderBuffers[i].lastUsbPressTime = 0;
        encoderBuffers[i].usbButtonPressed = false;
        encoderBuffers[i].currentDirection = 0;
    }
}

uint8_t createEncoderBufferEntry(uint8_t cwButtonId, uint8_t ccwButtonId) {
    if (bufferCount >= MAX_ENCODERS) {
        return 255; // Failed - buffer full
    }
    
    uint8_t index = bufferCount;
    encoderBuffers[index].cwButtonId = cwButtonId;
    encoderBuffers[index].ccwButtonId = ccwButtonId;
    encoderBuffers[index].pendingCwSteps = 0;
    encoderBuffers[index].pendingCcwSteps = 0;
    encoderBuffers[index].lastUsbPressTime = 0;
    encoderBuffers[index].usbButtonPressed = false;
    encoderBuffers[index].currentDirection = 0;
    
    bufferCount++;
    return index;
}

void addEncoderSteps(uint8_t buttonId, uint8_t steps) {
    for (uint8_t i = 0; i < bufferCount; i++) {
        bool isCw = (encoderBuffers[i].cwButtonId == buttonId);
        bool isCcw = (encoderBuffers[i].ccwButtonId == buttonId);
        
        if (isCw || isCcw) {
            // Prevent buffer overflow - cap at reasonable maximum
            uint8_t maxSteps = 50;
            
            if (isCw) {
                if (encoderBuffers[i].pendingCwSteps < maxSteps) {
                    encoderBuffers[i].pendingCwSteps += steps;
                    if (encoderBuffers[i].pendingCwSteps > maxSteps) {
                        encoderBuffers[i].pendingCwSteps = maxSteps;
                    }
                    

                }
            } else { // isCcw
                if (encoderBuffers[i].pendingCcwSteps < maxSteps) {
                    encoderBuffers[i].pendingCcwSteps += steps;
                    if (encoderBuffers[i].pendingCcwSteps > maxSteps) {
                        encoderBuffers[i].pendingCcwSteps = maxSteps;
                    }
                    

                }
            }
            break;
        }
    }
}

void processEncoderBuffers() {
    uint32_t currentTime = micros();
    
    for (uint8_t i = 0; i < bufferCount; i++) {
        EncoderBuffer& buffer = encoderBuffers[i];
        
        // Handle USB button release timing
        if (buffer.usbButtonPressed && (currentTime - buffer.lastUsbPressTime >= PRESS_DURATION_US)) {
            // Release the current button
            uint8_t currentButtonId = (buffer.currentDirection == 1) ? buffer.cwButtonId : buffer.ccwButtonId;
            uint8_t joyIdx = (currentButtonId > 0) ? (currentButtonId - 1) : 0;
            MyJoystick.setButton(joyIdx, 0);  // Release USB button
            buffer.usbButtonPressed = false;
            // DON'T reset currentDirection here - keep it for direction change detection
        }
        
        // Process buffer: decide which direction to process next
        if (!buffer.usbButtonPressed && (buffer.pendingCwSteps > 0 || buffer.pendingCcwSteps > 0)) {
            uint32_t timeSinceLastCycle = currentTime - buffer.lastUsbPressTime;
            
            // Determine which direction to process
            uint8_t nextDirection = 0;
            uint8_t nextButtonId = 0;
            
            // Priority logic: continue current direction until exhausted, then switch
            if (buffer.currentDirection == 1 && buffer.pendingCwSteps > 0) {
                // Continue CW if we have pending CW steps
                nextDirection = 1;
                nextButtonId = buffer.cwButtonId;
            } else if (buffer.currentDirection == 2 && buffer.pendingCcwSteps > 0) {
                // Continue CCW if we have pending CCW steps
                nextDirection = 2;
                nextButtonId = buffer.ccwButtonId;
            } else if (buffer.pendingCwSteps > 0) {
                // Switch to CW if current direction is exhausted
                nextDirection = 1;
                nextButtonId = buffer.cwButtonId;
            } else if (buffer.pendingCcwSteps > 0) {
                // Switch to CCW if current direction is exhausted
                nextDirection = 2;
                nextButtonId = buffer.ccwButtonId;
            }
            
            // Check timing constraints
            bool canProcess = false;
            if (buffer.lastUsbPressTime == 0) {
                // First press ever - allow immediate processing
                canProcess = true;
            } else if (nextDirection != buffer.currentDirection) {
                // Direction change - allow immediate processing
                canProcess = true;
            } else {
                // Same direction - need to wait for proper timing
                uint32_t fullCycleTime = PRESS_DURATION_US + PRESS_INTERVAL_US;
                if (timeSinceLastCycle >= fullCycleTime) {
                    canProcess = true;
                }
            }
            
            if (canProcess && nextDirection > 0) {
                uint8_t joyIdx = (nextButtonId > 0) ? (nextButtonId - 1) : 0;
                MyJoystick.setButton(joyIdx, 1);  // Press USB button
                buffer.usbButtonPressed = true;
                buffer.lastUsbPressTime = currentTime;
                buffer.currentDirection = nextDirection;
                
                // Decrement the appropriate counter
                if (nextDirection == 1) {
                    buffer.pendingCwSteps--;
                } else {
                    buffer.pendingCcwSteps--;
                }
            }
        }
        
        // Safety mechanism: if we have a stuck USB button for too long, force release
        if (buffer.usbButtonPressed && (currentTime - buffer.lastUsbPressTime >= PRESS_DURATION_US * 2)) {
            uint8_t currentButtonId = (buffer.currentDirection == 1) ? buffer.cwButtonId : buffer.ccwButtonId;
            uint8_t joyIdx = (currentButtonId > 0) ? (currentButtonId - 1) : 0;
            MyJoystick.setButton(joyIdx, 0);  // Force release
            buffer.usbButtonPressed = false;
            // DON'T reset currentDirection here either - keep it for direction tracking
        }
    }
}

uint8_t getEncoderBufferCount() {
    return bufferCount;
} 