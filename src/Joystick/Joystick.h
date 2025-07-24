/*
  Joystick.h

  Copyright (c) 2015-2017, Matthew Heironimus

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef JOYSTICK_h
#define JOYSTICK_h

#include "../DynamicHID/DynamicHID.h"
#include "../AnalogAxis.h"
#include "../Buttons.h"

#if ARDUINO < 10606
#error The Joystick library requires Arduino IDE 1.6.6 or greater. Please update your IDE.
#endif

#if ARDUINO > 10606
#if !defined(USBCON)
#error The Joystick library can only be used with a USB MCU (e.g. Arduino Leonardo, Arduino Micro, etc.).
#endif
#endif

#if !defined(_USING_DYNAMIC_HID)

#warning "Using legacy HID core (non pluggable)"

#else

#define JOYSTICK_DEFAULT_REPORT_ID         0x03
#define JOYSTICK_DEFAULT_BUTTON_COUNT        32
#define JOYSTICK_DEFAULT_HATSWITCH_COUNT      2
#define JOYSTICK_HATSWITCH_COUNT_MAXIMUM      2
#define JOYSTICK_HATSWITCH_RELEASE           -1
#define JOYSTICK_TYPE_JOYSTICK             0x04
#define JOYSTICK_TYPE_GAMEPAD              0x05
#define JOYSTICK_TYPE_MULTI_AXIS           0x08

class Joystick_
{
private:
    AnalogAxisManager _analogAxes;
    DigitalInputsManager _digitalInputs;
    
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    
    bool _autoSendState;
    uint8_t _hidReportId;
    uint8_t _hidReportSize;

protected:
    int buildAndSet16BitValue(bool includeValue, int32_t value, int32_t valueMinimum, int32_t valueMaximum, int32_t actualMinimum, int32_t actualMaximum, uint8_t dataLocation[]);
    int buildAndSetAxisValue(bool includeAxis, int32_t axisValue, int32_t axisMinimum, int32_t axisMaximum, uint8_t dataLocation[]);

public:
    Joystick_(
        uint8_t hidReportId = JOYSTICK_DEFAULT_REPORT_ID,
        uint8_t joystickType = JOYSTICK_TYPE_JOYSTICK,
        uint8_t buttonCount = JOYSTICK_DEFAULT_BUTTON_COUNT,
        uint8_t hatSwitchCount = JOYSTICK_DEFAULT_HATSWITCH_COUNT,
        bool includeXAxis = true,
        bool includeYAxis = true,
        bool includeZAxis = true,
        bool includeRxAxis = true,
        bool includeRyAxis = true,
        bool includeRzAxis = true,
        bool includeS1 = true,
        bool includeS2 = true);

    void begin(bool initAutoSendState = true);
    void end();
    
    void setAxis(uint8_t axis, int32_t value);
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum);
    void sendState();

    // Axis configuration
    inline void setAxisFilterLevel(uint8_t axis, AxisFilterLevel level) {
        _analogAxes.setAxisFilterLevel(axis, level);
    }
    inline void setAxisNoiseThreshold(uint8_t axis, int32_t threshold) {
        _analogAxes.setAxisNoiseThreshold(axis, threshold);
    }
    inline void setAxisResponseCurve(uint8_t axis, ResponseCurveType type) {
        _analogAxes.setAxisResponseCurve(axis, type);
    }
    inline void setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points) {
        _analogAxes.setAxisCustomCurve(axis, table, points);
    }
    inline void setAxisSmoothingFactor(uint8_t axis, uint8_t factor) {
        _analogAxes.setAxisSmoothingFactor(axis, factor);
    }
    inline void setAxisVelocityThreshold(uint8_t axis, int32_t threshold) {
        _analogAxes.setAxisVelocityThreshold(axis, threshold);
    }
    inline void setAxisEwmaAlpha(uint8_t axis, uint32_t alphaValue) {
        _analogAxes.setAxisEwmaAlpha(axis, alphaValue);
    }
    inline void setAxisDeadbandSize(uint8_t axis, int32_t size) {
        _analogAxes.setAxisDeadbandSize(axis, size);
    }
    
    // Digital inputs
    void setButton(uint8_t button, uint8_t value);
    void pressButton(uint8_t button);
    void releaseButton(uint8_t button);
    void setHatSwitch(int8_t hatSwitch, int16_t value);

    // Pin assignment
    inline void setAxisPin(uint8_t axis, int8_t pin) {
        _analogAxes.setAxisPin(axis, pin);
    }
    inline void readAllAxes() {
        _analogAxes.readAllAxes();
    }
};

#endif
#endif