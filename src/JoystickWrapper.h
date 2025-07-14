// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Teensy 4.0 uses the built-in joystick support from Teensyduino
// This wrapper provides the same interface as the Arduino Leonardo version
// but uses Teensy's native USB joystick functions

// Wrapper class that provides the same interface as the Arduino Leonardo version
// but uses Teensy's native USB joystick functions
class Joystick_ {
private:
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    bool _autoSendState;
    
public:
    // Constructor with compatible signature to the original
    Joystick_(
        uint8_t hidReportId = 0x03,          // Not used on Teensy, but kept for compatibility
        uint8_t joystickType = 0x04,         // Not used on Teensy, but kept for compatibility  
        uint8_t buttonCount = 32,
        uint8_t hatSwitchCount = 2,          // Teensy supports 1 hat switch
        bool includeXAxis = true,
        bool includeYAxis = true,
        bool includeZAxis = true,
        bool includeRxAxis = true,
        bool includeRyAxis = true,
        bool includeRzAxis = true,
        bool includeS1 = true,
        bool includeS2 = true
    ) : _buttonCount(buttonCount), _hatSwitchCount(hatSwitchCount), _autoSendState(true) {
        // Teensy automatically handles axis configuration based on USB type
    }
    
    void begin(bool initAutoSendState = true) {
        _autoSendState = initAutoSendState;
        Joystick.useManualSend(!_autoSendState);
        delay(100); // Give USB time to initialize
    }
    
    void end() {
        // Not implemented in Teensy's library, but kept for compatibility
    }
    
    // Button functions
    void setButton(uint8_t button, uint8_t value) {
        if (button >= _buttonCount) return;
        Joystick.button(button + 1, value); // Teensy uses 1-based button numbering
    }
    
    void pressButton(uint8_t button) {
        setButton(button, 1);
    }
    
    void releaseButton(uint8_t button) {
        setButton(button, 0);
    }
    
    // Axis functions - convert from our 32-bit range to Teensy's 16-bit range
    void setAxis(uint8_t axis, int32_t value) {
        // Convert to 0-1023 range that Teensy expects
        uint16_t teensy_value = constrain(value, 0, 1023);
        
        switch(axis) {
            case 0: // X axis
                Joystick.X(teensy_value);
                break;
            case 1: // Y axis  
                Joystick.Y(teensy_value);
                break;
            case 2: // Z axis
                Joystick.Z(teensy_value);
                break;
            case 3: // Z rotation
                Joystick.Zrotate(teensy_value);
                break;
            case 4: // Left slider
                Joystick.sliderLeft(teensy_value);
                break;
            case 5: // Right slider
                Joystick.sliderRight(teensy_value);
                break;
        }
    }
    
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum) {
        // Teensy handles range automatically, but we store for potential future use
        // For now, we assume 0-1023 range which is Teensy's default
    }
    
    // Hat switch function
    void setHatSwitch(int8_t hatSwitchIndex, int16_t value) {
        if (hatSwitchIndex != 0) return; // Teensy supports only 1 hat switch
        Joystick.hat(value);
    }
    
    void sendState() {
        if (!_autoSendState) {
            Joystick.send_now();
        }
    }
    
    // Additional compatibility functions for axis configuration
    void setAxisFilterLevel(uint8_t axis, int level) { /* Not implemented on Teensy */ }
    void setAxisNoiseThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on Teensy */ }
    void setAxisResponseCurve(uint8_t axis, int type) { /* Not implemented on Teensy */ }
    void setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points) { /* Not implemented on Teensy */ }
    void setAxisSmoothingFactor(uint8_t axis, uint8_t factor) { /* Not implemented on Teensy */ }
    void setAxisVelocityThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on Teensy */ }
    void setAxisPin(uint8_t axis, int8_t pin) { /* Not implemented on Teensy */ }
    void readAllAxes() { /* Not implemented on Teensy */ }
};

extern Joystick_ MyJoystick;
