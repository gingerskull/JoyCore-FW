// SPDX-License-Identifier: GPL-3.0-or-later
#include "HIDMapping.h"
#include "TinyUSBGamepad.h"
#include "../../config/core/ConfigManager.h"
#include "../../utils/Debug.h"
#include "../../Config.h"
#include <string.h>

// Static member definitions
HIDMappingInfo HIDMappingManager::_mappingInfo;
uint8_t HIDMappingManager::_buttonMapping[128];
bool HIDMappingManager::_mappingValid = false;

// Self-test manager state
static SelfTestControl _selfTestState = {0};
static uint32_t _selfTestLastUpdate = 0;
static TinyUSBGamepad* _gamepadForTest = nullptr;

// CRC16-CCITT calculation
uint16_t calculateMappingCRC16(const uint8_t* mapping, uint8_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint8_t i = 0; i < length; i++) {
        crc ^= (uint16_t)mapping[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

// Check if mapping is sequential
bool isMappingSequential(const uint8_t* mapping, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        if (mapping[i] != i) {
            return false;
        }
    }
    return true;
}

void HIDMappingManager::initialize() {
    _gamepadForTest = &MyGamepad;
    updateFromConfig();
}

void HIDMappingManager::updateFromConfig() {
    buildMappingInfo();
    buildButtonMapping();
    _mappingValid = true;
}

void HIDMappingManager::buildMappingInfo() {
    memset(&_mappingInfo, 0, sizeof(_mappingInfo));
    
    _mappingInfo.protocol_version = HID_MAPPING_PROTOCOL_VERSION;
    _mappingInfo.report_id = 1; // Gamepad report ID
    
    // Get counts from configuration
    const LogicalInput* inputs = g_configManager.getLogicalInputs();
    uint8_t inputCount = g_configManager.getLogicalInputCount();
    
    // Count actual buttons (exclude encoders and other non-button inputs)
    uint8_t buttonCount = 0;
    for (uint8_t i = 0; i < inputCount; i++) {
        if (inputs[i].type == INPUT_PIN || 
            inputs[i].type == INPUT_MATRIX || 
            inputs[i].type == INPUT_SHIFTREG) {
            buttonCount++;
        }
    }
    
    _mappingInfo.button_count = buttonCount;
    
    // Count enabled axes
    uint8_t axisCount = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (g_configManager.getAxisConfig(i)->enabled) {
            axisCount++;
        }
    }
    _mappingInfo.axis_count = axisCount;
    
    // Set fixed offsets based on HID report structure
    _mappingInfo.button_byte_offset = 0;  // Buttons start at byte 0
    _mappingInfo.button_bit_order = HID_MAPPING_BIT_ORDER_LSB;  // LSB first per byte
    _mappingInfo.frame_counter_offset = 48;  // After 16 bytes buttons + 32 bytes axes
    
    // Build button mapping and calculate CRC
    buildButtonMapping();
    
    if (isMappingSequential(_buttonMapping, buttonCount)) {
        _mappingInfo.mapping_crc = 0x0000;  // Signal sequential mapping
    } else {
        _mappingInfo.mapping_crc = calculateMappingCRC16(_buttonMapping, buttonCount);
    }
}

void HIDMappingManager::buildButtonMapping() {
    memset(_buttonMapping, 0, sizeof(_buttonMapping));
    
    const LogicalInput* inputs = g_configManager.getLogicalInputs();
    uint8_t inputCount = g_configManager.getLogicalInputCount();
    
    uint8_t buttonIndex = 0;
    for (uint8_t i = 0; i < inputCount && buttonIndex < 128; i++) {
        uint8_t joyButtonID = 0;
        switch (inputs[i].type) {
            case INPUT_PIN:
                joyButtonID = inputs[i].u.pin.joyButtonID;
                break;
            case INPUT_MATRIX:
                joyButtonID = inputs[i].u.matrix.joyButtonID;
                break;
            case INPUT_SHIFTREG:
                joyButtonID = inputs[i].u.shiftreg.joyButtonID;
                break;
            default:
                continue; // Skip non-button inputs
        }
        _buttonMapping[buttonIndex] = joyButtonID;
        buttonIndex++;
    }
}

uint16_t HIDMappingManager::handleGetMappingInfo(uint8_t* buffer, uint16_t reqlen) {
    if (reqlen < sizeof(HIDMappingInfo)) {
        return 0;
    }
    
    if (!_mappingValid) {
        updateFromConfig();
    }
    
    memcpy(buffer, &_mappingInfo, sizeof(HIDMappingInfo));
    return sizeof(HIDMappingInfo);
}

uint16_t HIDMappingManager::handleGetButtonMap(uint8_t* buffer, uint16_t reqlen) {
    if (!_mappingValid) {
        updateFromConfig();
    }
    
    // If mapping is sequential, return empty (signaled by CRC=0x0000)
    if (_mappingInfo.mapping_crc == 0x0000) {
        return 0;
    }
    
    uint16_t copySize = min((uint16_t)_mappingInfo.button_count, reqlen);
    memcpy(buffer, _buttonMapping, copySize);
    return copySize;
}

uint16_t HIDMappingManager::handleGetSelfTest(uint8_t* buffer, uint16_t reqlen) {
    if (reqlen < sizeof(SelfTestControl)) {
        return 0;
    }
    
    // Update self-test state if running
    if (_selfTestState.status == SELFTEST_STATUS_RUNNING) {
        uint32_t now = millis();
        if (now - _selfTestLastUpdate >= _selfTestState.interval_ms) {
            _selfTestLastUpdate = now;
            
            // Clear previous button
            if (_selfTestState.current_button > 0) {
                _gamepadForTest->releaseButton(_selfTestState.current_button - 1);
            }
            
            // Set next button
            if (_selfTestState.current_button < _mappingInfo.button_count) {
                _gamepadForTest->pressButton(_selfTestState.current_button);
                _selfTestState.current_button++;
            } else {
                // Test complete
                _selfTestState.status = SELFTEST_STATUS_COMPLETE;
                _selfTestState.command = SELFTEST_CMD_STOP;
            }
        }
    }
    
    memcpy(buffer, &_selfTestState, sizeof(SelfTestControl));
    return sizeof(SelfTestControl);
}

void HIDMappingManager::handleSetSelfTest(const uint8_t* buffer, uint16_t bufsize) {
    if (bufsize < sizeof(SelfTestControl)) {
        return;
    }
    
    SelfTestControl newState;
    memcpy(&newState, buffer, sizeof(SelfTestControl));
    
    switch (newState.command) {
        case SELFTEST_CMD_START_WALK:
            // Clear all buttons first
            for (uint8_t i = 0; i < 128; i++) {
                _gamepadForTest->releaseButton(i);
            }
            
            _selfTestState.command = SELFTEST_CMD_START_WALK;
            _selfTestState.status = SELFTEST_STATUS_RUNNING;
            _selfTestState.current_button = 0;
            _selfTestState.interval_ms = (newState.interval_ms >= 30 && newState.interval_ms <= 50) ? 
                                        newState.interval_ms : SELFTEST_DEFAULT_INTERVAL_MS;
            _selfTestLastUpdate = millis();
            break;
            
        case SELFTEST_CMD_STOP:
            // Clear all buttons
            for (uint8_t i = 0; i < 128; i++) {
                _gamepadForTest->releaseButton(i);
            }
            
            _selfTestState.command = SELFTEST_CMD_STOP;
            _selfTestState.status = SELFTEST_STATUS_IDLE;
            _selfTestState.current_button = 0;
            break;
    }
}