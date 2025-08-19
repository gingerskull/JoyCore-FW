// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stdint.h>

// HID Feature Report IDs for mapping information
#define HID_FEATURE_MAPPING_INFO    3   // HIDMappingInfo structure
#define HID_FEATURE_BUTTON_MAP      4   // Button mapping array
#define HID_FEATURE_SELFTEST        5   // Self-test control

// HID Mapping Info Structure (little-endian)
typedef struct __attribute__((packed)) {
    uint8_t protocol_version;       // Protocol version (start at 1)
    uint8_t report_id;              // Input report ID (1 for gamepad)
    uint8_t button_count;           // Number of buttons configured
    uint8_t axis_count;             // Number of axes configured
    uint8_t button_byte_offset;     // Byte offset where buttons start in input report
    uint8_t button_bit_order;       // 0 = LSB-first-per-byte, 1 = MSB-first
    uint16_t mapping_crc;           // CRC16 of button mapping (0x0000 = sequential)
    uint8_t frame_counter_offset;   // Byte offset of frame counter in input report
    uint8_t reserved[7];            // Reserved for future use
} HIDMappingInfo;

// Self-test control structure
typedef struct __attribute__((packed)) {
    uint8_t command;                // 0 = stop, 1 = start button walk
    uint8_t status;                 // 0 = idle, 1 = running, 2 = complete
    uint8_t current_button;         // Current button being tested (0-127)
    uint8_t interval_ms;            // Test interval in milliseconds (30-50)
    uint8_t reserved[4];            // Reserved for future use
} SelfTestControl;

// Self-test commands
#define SELFTEST_CMD_STOP           0
#define SELFTEST_CMD_START_WALK     1

// Self-test status values
#define SELFTEST_STATUS_IDLE        0
#define SELFTEST_STATUS_RUNNING     1
#define SELFTEST_STATUS_COMPLETE    2

// Constants
#define HID_MAPPING_PROTOCOL_VERSION    1
#define HID_MAPPING_BIT_ORDER_LSB       0
#define HID_MAPPING_BIT_ORDER_MSB       1
#define SELFTEST_DEFAULT_INTERVAL_MS    40

// Maximum sizes
#define HID_MAPPING_INFO_SIZE           16
#define HID_BUTTON_MAP_MAX_SIZE         128
#define HID_SELFTEST_CONTROL_SIZE       8

// CRC16-CCITT calculation for button mapping
uint16_t calculateMappingCRC16(const uint8_t* mapping, uint8_t length);

// Check if button mapping is sequential (bit index == joyButtonID)
bool isMappingSequential(const uint8_t* mapping, uint8_t length);

// HID Mapping Manager class
class HIDMappingManager {
public:
    // Initialize with current configuration
    static void initialize();
    
    // Update mapping info when configuration changes
    static void updateFromConfig();
    
    // Feature report handlers
    static uint16_t handleGetMappingInfo(uint8_t* buffer, uint16_t reqlen);
    static uint16_t handleGetButtonMap(uint8_t* buffer, uint16_t reqlen);
    static uint16_t handleGetSelfTest(uint8_t* buffer, uint16_t reqlen);
    static void handleSetSelfTest(const uint8_t* buffer, uint16_t bufsize);
    
    // Get current mapping info
    static const HIDMappingInfo* getMappingInfo() { return &_mappingInfo; }
    
private:
    static HIDMappingInfo _mappingInfo;
    static uint8_t _buttonMapping[128];
    static bool _mappingValid;
    
    // Helper methods
    static void buildMappingInfo();
    static void buildButtonMapping();
};