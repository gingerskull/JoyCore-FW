// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// RP2040 uses the Adafruit TinyUSB library for USB HID support
// This wrapper provides the same interface as the Arduino Leonardo/Teensy versions
// but uses TinyUSB's HID gamepad functions

#include <Adafruit_TinyUSB.h>

// USB HID report descriptor using TinyUSB's helper
// Gamepad with 32 buttons, 6 axes, and 1 hat switch
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_GAMEPAD()
};

// HID Gamepad object for TinyUSB
Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false);

// Wrapper class that provides the same interface as the Arduino Leonardo/Teensy versions
// but uses TinyUSB's HID gamepad functions
class Joystick_ {
private:
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    bool _autoSendState;
    
    // Gamepad report structure
    hid_gamepad_report_t _report;
    
public:
    // Constructor with compatible signature to the original
    Joystick_(
        uint8_t hidReportId = 0x03,          // Not used on RP2040, but kept for compatibility
        uint8_t joystickType = 0x04,         // Not used on RP2040, but kept for compatibility  
        uint8_t buttonCount = 32,
        uint8_t hatSwitchCount = 2,          // TinyUSB supports 1 hat switch
        bool includeXAxis = true,
        bool includeYAxis = true,
        bool includeZAxis = true,
        bool includeRxAxis = true,
        bool includeRyAxis = true,
        bool includeRzAxis = true,
        bool includeS1 = true,
        bool includeS2 = true
    ) : _buttonCount(buttonCount), _hatSwitchCount(hatSwitchCount), _autoSendState(true) {
        // Initialize report structure
        memset(&_report, 0, sizeof(_report));
    }
    
    void begin(bool initAutoSendState = true) {
        _autoSendState = initAutoSendState;
        
        // Initialize TinyUSB
        TinyUSB_Device_Init(0);
        usb_hid.begin();
        
        // Wait until device mounted
        while (!TinyUSBDevice.mounted()) {
            delay(1);
        }
        
        delay(100); // Give USB time to initialize
    }
    
    void end() {
        // Not implemented in TinyUSB, but kept for compatibility
    }
    
    // Button functions
    void setButton(uint8_t button, uint8_t value) {
        if (button >= _buttonCount) return;
        
        // TinyUSB uses bitmask for buttons (up to 32 buttons)
        if (value) {
            _report.buttons |= (1UL << button);
        } else {
            _report.buttons &= ~(1UL << button);
        }
        
        if (_autoSendState) {
            sendState();
        }
    }
    
    void pressButton(uint8_t button) {
        setButton(button, 1);
    }
    
    void releaseButton(uint8_t button) {
        setButton(button, 0);
    }
    
    // Axis functions - convert from our 32-bit range to TinyUSB's 8-bit signed range
    void setAxis(uint8_t axis, int32_t value) {
        // Convert to -127 to 127 range that TinyUSB expects
        // Input is 0-1023, map to -127 to 127
        int8_t tinyusb_value = map(constrain(value, 0, 1023), 0, 1023, -127, 127);
        
        switch(axis) {
            case 0: // X axis
                _report.x = tinyusb_value;
                break;
            case 1: // Y axis  
                _report.y = tinyusb_value;
                break;
            case 2: // Z axis (mapped to rz for TinyUSB)
                _report.rz = tinyusb_value;
                break;
            case 3: // Z rotation (mapped to rx)
                _report.rx = tinyusb_value;
                break;
            case 4: // Left slider (mapped to ry)
                _report.ry = tinyusb_value;
                break;
            case 5: // Right slider (mapped to z)
                _report.z = tinyusb_value;
                break;
        }
        
        if (_autoSendState) {
            sendState();
        }
    }
    
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum) {
        // TinyUSB handles range automatically, but we store for potential future use
        // For now, we assume -127 to 127 range which is TinyUSB's default
    }
    
    // Hat switch function
    void setHatSwitch(int8_t hatSwitchIndex, int16_t value) {
        if (hatSwitchIndex != 0) return; // TinyUSB supports only 1 hat switch
        
        // Convert angle to TinyUSB hat format
        uint8_t hat_value = GAMEPAD_HAT_CENTERED;
        
        if (value >= 0 && value <= 360) {
            // Convert degrees to 8-way hat
            if (value >= 337 || value < 23) hat_value = GAMEPAD_HAT_UP;
            else if (value >= 23 && value < 68) hat_value = GAMEPAD_HAT_UP_RIGHT;
            else if (value >= 68 && value < 113) hat_value = GAMEPAD_HAT_RIGHT;
            else if (value >= 113 && value < 158) hat_value = GAMEPAD_HAT_DOWN_RIGHT;
            else if (value >= 158 && value < 203) hat_value = GAMEPAD_HAT_DOWN;
            else if (value >= 203 && value < 248) hat_value = GAMEPAD_HAT_DOWN_LEFT;
            else if (value >= 248 && value < 293) hat_value = GAMEPAD_HAT_LEFT;
            else if (value >= 293 && value < 337) hat_value = GAMEPAD_HAT_UP_LEFT;
        }
        
        _report.hat = hat_value;
        
        if (_autoSendState) {
            sendState();
        }
    }
    
    void sendState() {
        // Send the report via TinyUSB
        if (usb_hid.ready()) {
            usb_hid.sendReport(0, &_report, sizeof(_report));
        }
    }
    
    // Additional compatibility functions for axis configuration
    void setAxisFilterLevel(uint8_t axis, int level) { /* Not implemented on RP2040 */ }
    void setAxisNoiseThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on RP2040 */ }
    void setAxisResponseCurve(uint8_t axis, int type) { /* Not implemented on RP2040 */ }
    void setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points) { /* Not implemented on RP2040 */ }
    void setAxisSmoothingFactor(uint8_t axis, uint8_t factor) { /* Not implemented on RP2040 */ }
    void setAxisVelocityThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on RP2040 */ }
    void setAxisPin(uint8_t axis, int8_t pin) { /* Not implemented on RP2040 */ }
    void readAllAxes() { /* Not implemented on RP2040 */ }
};

extern Joystick_ MyJoystick;
