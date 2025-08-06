// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// RP2040 Raspberry Pi Pico uses TinyUSB for optimized HID performance
// This wrapper provides the same interface as the Teensy version
// but uses our optimized TinyUSBGamepad implementation

#include "hid/TinyUSBGamepad.h"

// Wrapper class that provides the same interface as the Teensy version
// but uses our optimized TinyUSBGamepad implementation
class Joystick_ {
private:
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    bool _autoSendState;
    TinyUSBGamepad* _gamepad;
    
public:
    // Constructor with compatible signature to the original
    Joystick_(
        uint8_t hidReportId = 0x03,          // Not used with TinyUSB, but kept for compatibility
        uint8_t joystickType = 0x04,         // Not used with TinyUSB, but kept for compatibility  
        uint8_t buttonCount = 128,           // TinyUSBGamepad supports up to 128 buttons
        uint8_t hatSwitchCount = 4,          // TinyUSBGamepad supports 4 hat switches
        bool includeXAxis = true,
        bool includeYAxis = true,
        bool includeZAxis = true,
        bool includeRxAxis = true,
        bool includeRyAxis = true,
        bool includeRzAxis = true,
        bool includeS1 = true,
        bool includeS2 = true
    ) : _buttonCount(buttonCount), _hatSwitchCount(hatSwitchCount), _autoSendState(true) {
        // Use global MyGamepad instance for optimized performance
        _gamepad = &MyGamepad;
    }
    
    void begin(bool initAutoSendState = true) {
        _autoSendState = initAutoSendState;
        // Initialize TinyUSBGamepad with auto-send preference
        _gamepad->begin(_autoSendState);
    }
    
    void end() {
        // End TinyUSBGamepad operation
        _gamepad->end();
    }
    
    // Button functions
    void setButton(uint8_t button, uint8_t value) {
        if (button >= _buttonCount) {
            return;
        }
        
        _gamepad->setButton(button, value != 0);
    }
    
    void pressButton(uint8_t button) {
        setButton(button, 1);
    }
    
    void releaseButton(uint8_t button) {
        setButton(button, 0);
    }
    
    // Axis functions - convert from our 32-bit range to TinyUSB's 16-bit range
    void setAxis(uint8_t axis, int32_t value) {
        // Convert to -32767 to 32767 range that TinyUSBGamepad expects
        int16_t gamepad_value = constrain(value, -32767, 32767);
        
        // TinyUSBGamepad supports up to 16 axes directly
        if (axis < 16) {
            _gamepad->setAxis(axis, gamepad_value);
        }
    }
    
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum) {
        // TinyUSBGamepad uses fixed -32767 to 32767 range internally
        // This function is kept for compatibility but doesn't change behavior
    }
    
    // Hat switch function
    void setHatSwitch(int8_t hatSwitchIndex, int16_t value) {
        if (hatSwitchIndex < 0 || hatSwitchIndex >= 4) return; // TinyUSBGamepad supports 4 hat switches
        
        int8_t hatValue;
        if (value < 0) {
            hatValue = -1; // Center/neutral (converted internally to HAT_DIR_C)
        } else {
            // Convert from degrees to hat direction
            // 0=N, 45=NE, 90=E, 135=SE, 180=S, 225=SW, 270=W, 315=NW
            hatValue = (value / 45) % 8;
        }
        
        _gamepad->setHat(hatSwitchIndex, hatValue);
    }
    
    void sendState() {
        _gamepad->sendReport();
    }
    
    // Auto-send control for MOMENTARY button handling
    void setAutoSend(bool autoSend) {
        _gamepad->setAutoSend(autoSend);
    }
    
    bool getAutoSend() const {
        return _gamepad->getAutoSend();
    }

    // USB device descriptor configuration (must be called before begin())
    void setUSBDescriptor(uint16_t vid, uint16_t pid, const char* manufacturer = nullptr, const char* product = nullptr) {
        TinyUSBGamepad::setUSBDescriptor(vid, pid, manufacturer, product);
    }

};

#ifdef DEFINE_MYJOYSTICK
extern Joystick_ MyJoystick;
#else
extern Joystick_ MyJoystick;
#endif
