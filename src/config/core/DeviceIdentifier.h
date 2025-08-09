// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>
#include <stddef.h>  // for size_t
#include <stdio.h>   // for snprintf
#include <string.h>  // for strcmp
#include "ConfigMode.h"

// Fixed device identification for JoyCore-FW
// This identifier NEVER changes and allows configuration programs
// to reliably detect JoyCore-FW devices regardless of USB VID/PID settings

namespace JoyCore {
    
    // FIXED device signature - THIS MUST NEVER CHANGE
    // Config programs will look for this exact string to identify JoyCore-FW boards
    constexpr const char* DEVICE_SIGNATURE = "JOYCORE-FW";
    
    // FIXED magic identifier - THIS MUST NEVER CHANGE  
    // Hex representation of "JOYC" - used as secondary verification
    constexpr uint32_t DEVICE_MAGIC = 0x4A4F5943;
    
    // Serial command for device identification
    constexpr const char* IDENTIFY_COMMAND = "IDENTIFY";
    
    // Fixed response prefix for easy parsing
    constexpr const char* IDENTIFY_RESPONSE_PREFIX = "JOYCORE_ID";
    
    // Format the identification response
    // Fixed format: "JOYCORE_ID:JOYCORE-FW:4A4F5943:FW_VERSION"
    // The first three parts are FIXED, only firmware version changes
    inline void formatIdentifyResponse(char* buffer, size_t bufferSize) {
        snprintf(buffer, bufferSize, 
                "%s:%s:%08X:%u",
                IDENTIFY_RESPONSE_PREFIX,  // "JOYCORE_ID" - fixed
                DEVICE_SIGNATURE,          // "JOYCORE-FW" - fixed
                DEVICE_MAGIC,              // "4A4F5943" - fixed
                FIRMWARE_VERSION);         // Current firmware version (can change)
    }
    
    // Check if a command is the IDENTIFY command
    inline bool isIdentifyCommand(const char* command) {
        return strcmp(command, IDENTIFY_COMMAND) == 0;
    }
}

// Example response: "JOYCORE_ID:JOYCORE-FW:4A4F5943:12"
// Config program pseudocode:
//   1. Send "IDENTIFY\n" to serial port
//   2. Read response
//   3. Check if response starts with "JOYCORE_ID:JOYCORE-FW:4A4F5943:"
//   4. If yes, it's a JoyCore-FW board (firmware version follows the fixed parts)