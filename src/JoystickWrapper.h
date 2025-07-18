// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// RP2040 Raspberry Pi Pico uses PicoGamepad library
// This wrapper provides the same interface as the Teensy version
// but uses PicoGamepad functions

#include "PicoGamepad.h"

using namespace arduino;

// Wrapper class that provides the same interface as the Teensy version
// but uses PicoGamepad functions
class Joystick_ {
private:
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    bool _autoSendState;
    PicoGamepad _gamepad;
    
public:
    // Constructor with compatible signature to the original
    Joystick_(
        uint8_t hidReportId = 0x03,          // Not used on RP2040, but kept for compatibility
        uint8_t joystickType = 0x04,         // Not used on RP2040, but kept for compatibility  
        uint8_t buttonCount = 32,
        uint8_t hatSwitchCount = 2,          // PicoGamepad supports 4 hat switches
        bool includeXAxis = true,
        bool includeYAxis = true,
        bool includeZAxis = true,
        bool includeRxAxis = true,
        bool includeRyAxis = true,
        bool includeRzAxis = true,
        bool includeS1 = true,
        bool includeS2 = true
    ) : _buttonCount(buttonCount), _hatSwitchCount(hatSwitchCount), _autoSendState(true), _gamepad() {
        // PicoGamepad handles axis configuration automatically
    }
    
    void begin(bool initAutoSendState = true) {
        _autoSendState = initAutoSendState;
        // PicoGamepad initializes automatically through USB
        delay(100); // Give USB time to initialize
    }
    
    void end() {
        // Not implemented in PicoGamepad, but kept for compatibility
    }
    
    // Button functions
    void setButton(uint8_t button, uint8_t value) {
        if (button >= _buttonCount) {
            Serial.print("ERROR: Button "); Serial.print(button); 
            Serial.print(" >= max count "); Serial.println(_buttonCount);
            return;
        }
        
        _gamepad.SetButton(button, value != 0); // PicoGamepad uses 0-based button numbering
    }
    
    void pressButton(uint8_t button) {
        setButton(button, 1);
    }
    
    void releaseButton(uint8_t button) {
        setButton(button, 0);
    }
    
    // Axis functions - convert from our 32-bit range to PicoGamepad's 16-bit range
    void setAxis(uint8_t axis, int32_t value) {
        // Convert to -32767 to 32767 range that PicoGamepad expects
        int16_t pico_value = constrain(value, -32767, 32767);
        
        switch(axis) {
            case 0: // X axis
                _gamepad.SetX(pico_value);
                break;
            case 1: // Y axis  
                _gamepad.SetY(pico_value);
                break;
            case 2: // Z axis
                _gamepad.SetZ(pico_value);
                break;
            case 3: // Rx axis
                _gamepad.SetRx(pico_value);
                break;
            case 4: // Ry axis
                _gamepad.SetRy(pico_value);
                break;
            case 5: // Rz axis
                _gamepad.SetRz(pico_value);
                break;
            case 6: // Slider 1
                _gamepad.SetSlider(pico_value);
                break;
            case 7: // Slider 2
                _gamepad.SetDial(pico_value);
                break;
        }
    }
    
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum) {
        // PicoGamepad handles range internally, but we store for potential future use
        // For now, we assume -32767 to 32767 range which is PicoGamepad's default
    }
    
    // Hat switch function
    void setHatSwitch(int8_t hatSwitchIndex, int16_t value) {
        if (hatSwitchIndex < 0 || hatSwitchIndex >= 4) return; // PicoGamepad supports 4 hat switches
        
        uint8_t hatValue;
        if (value < 0) {
            hatValue = HAT_DIR_C; // Center/neutral
        } else {
            // Convert from degrees to hat direction
            // 0=N, 45=NE, 90=E, 135=SE, 180=S, 225=SW, 270=W, 315=NW
            hatValue = (value / 45) % 8;
        }
        
        _gamepad.SetHat(hatSwitchIndex, hatValue);
    }
    
    void sendState() {
        _gamepad.send_update();
    }
    
    // Additional compatibility functions for axis configuration
    void setAxisFilterLevel(uint8_t axis, int level) { /* Not implemented on PicoGamepad */ }
    void setAxisNoiseThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on PicoGamepad */ }
    void setAxisResponseCurve(uint8_t axis, int type) { /* Not implemented on PicoGamepad */ }
    void setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points) { /* Not implemented on PicoGamepad */ }
    void setAxisSmoothingFactor(uint8_t axis, uint8_t factor) { /* Not implemented on PicoGamepad */ }
    void setAxisVelocityThreshold(uint8_t axis, int32_t threshold) { /* Not implemented on PicoGamepad */ }
    void setAxisPin(uint8_t axis, int8_t pin) { /* Not implemented on PicoGamepad */ }
    void readAllAxes() { /* Not implemented on PicoGamepad */ }
};

extern Joystick_ MyJoystick;
