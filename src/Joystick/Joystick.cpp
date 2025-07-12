/*
  Joystick.cpp

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

#include "Joystick.h"

#if defined(_USING_DYNAMIC_HID)

#define JOYSTICK_AXIS_MINIMUM 0
#define JOYSTICK_AXIS_MAXIMUM 65535

Joystick_::Joystick_(
	uint8_t hidReportId,
	uint8_t joystickType,
    uint8_t buttonCount,
	uint8_t hatSwitchCount,
	bool includeXAxis,
	bool includeYAxis,
	bool includeZAxis,
	bool includeRxAxis,
	bool includeRyAxis,
	bool includeRzAxis,
	bool includeS1,
	bool includeS2)
  : _digitalInputs(buttonCount,hatSwitchCount)
{
    _hidReportId = hidReportId;
    _buttonCount = buttonCount;
	_hatSwitchCount = hatSwitchCount;
	
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_X, includeXAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_Y, includeYAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_Z, includeZAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_RX, includeRxAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_RY, includeRyAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_RZ, includeRzAxis);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_S1, includeS1);
	_analogAxes.enableAxis(AnalogAxisManager::AXIS_S2, includeS2);
	
    // Build HID Report Descriptor
	uint8_t buttonsInLastByte = _buttonCount % 8;
	uint8_t buttonPaddingBits = 0;
	if (buttonsInLastByte > 0) buttonPaddingBits = 8 - buttonsInLastByte;
	uint8_t totalAxes = _analogAxes.getAxisCount();
		
    uint8_t tempHidReportDescriptor[150];
    int hidReportDescriptorSize = 0;

    // USAGE_PAGE (Generic Desktop)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // USAGE (Joystick - 0x04; Gamepad - 0x05; Multi-axis Controller - 0x08)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
    tempHidReportDescriptor[hidReportDescriptorSize++] = joystickType;

    // COLLECTION (Application)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xa1;
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

    // REPORT_ID (Default: 3)
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0x85;
    tempHidReportDescriptor[hidReportDescriptorSize++] = _hidReportId;
	
	if (_buttonCount > 0) {
		// USAGE_PAGE (Button)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;

		// USAGE_MINIMUM (Button 1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x19;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// USAGE_MAXIMUM (Button count)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x29;
		tempHidReportDescriptor[hidReportDescriptorSize++] = _buttonCount;

		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x25;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// REPORT_SIZE (1)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// REPORT_COUNT (# of buttons)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = _buttonCount;

		// UNIT_EXPONENT (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x55;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// UNIT (None)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x65;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;

		if (buttonPaddingBits > 0) {
			// REPORT_SIZE (1)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

			// REPORT_COUNT (# of padding bits)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
			tempHidReportDescriptor[hidReportDescriptorSize++] = buttonPaddingBits;
					
			// INPUT (Const,Var,Abs)
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x03;
		}
	}

	if (totalAxes > 0) {
		// USAGE_PAGE (Generic Desktop)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x05;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;
		
		// USAGE (Pointer)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x01;

		// LOGICAL_MINIMUM (0)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x15;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// LOGICAL_MAXIMUM (65535)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x27;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0XFF;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// REPORT_SIZE (16)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x75;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x10;

		// REPORT_COUNT (totalAxes)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x95;
		tempHidReportDescriptor[hidReportDescriptorSize++] = totalAxes;
						
		// COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xA1;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x00;

		// Add axis usage codes
		if (includeXAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x30;
		}
		if (includeYAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x31;
		}
		if (includeZAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x32;
		}
		if (includeRxAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x33;
		}
		if (includeRyAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x34;
		}
		if (includeRzAxis) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x35;
		}
		if (includeS1) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBA;
		}
		if (includeS2) {
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0x09;
			tempHidReportDescriptor[hidReportDescriptorSize++] = 0xBB;
		}
		
		// INPUT (Data,Var,Abs)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x81;
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0x02;
		
		// END_COLLECTION (Physical)
		tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;
	}

    // END_COLLECTION
    tempHidReportDescriptor[hidReportDescriptorSize++] = 0xc0;

	uint8_t *customHidReportDescriptor = new uint8_t[hidReportDescriptorSize];
	memcpy(customHidReportDescriptor, tempHidReportDescriptor, hidReportDescriptorSize);
	
	DynamicHIDSubDescriptor *node = new DynamicHIDSubDescriptor(customHidReportDescriptor, hidReportDescriptorSize, false);
	DynamicHID().AppendDescriptor(node);
	
	_hidReportSize = _digitalInputs.getButtonValuesArraySize();
	_hidReportSize += (_hatSwitchCount > 0);
	_hidReportSize += (totalAxes * 2);
}

void Joystick_::begin(bool initAutoSendState)
{
	delay(2000);
	_autoSendState = initAutoSendState;
	if (initAutoSendState) {
		delay(200);
		sendState();
	}
}

void Joystick_::end() {}

void Joystick_::setButton(uint8_t button, uint8_t value)
{
    _digitalInputs.setButton(button, value);
    if (_autoSendState) sendState();
}

void Joystick_::pressButton(uint8_t button)
{
    _digitalInputs.setButton(button, 1);
    if (_autoSendState) sendState();
}

void Joystick_::releaseButton(uint8_t button)
{
    _digitalInputs.setButton(button, 0);
    if (_autoSendState) sendState();
}

void Joystick_::setAxis(uint8_t axis, int32_t value)
{
    _analogAxes.processAxisValue(axis, value);
    if (_autoSendState) sendState();
}

void Joystick_::setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum)
{
    _analogAxes.setAxisRange(axis, minimum, maximum);
}

void Joystick_::setHatSwitch(int8_t hatSwitchIndex, int16_t value)
{
    _digitalInputs.setHatSwitch(hatSwitchIndex, value);
    if (_autoSendState) sendState();
}

int Joystick_::buildAndSet16BitValue(bool includeValue, int32_t value, int32_t valueMinimum, int32_t valueMaximum, int32_t actualMinimum, int32_t actualMaximum, uint8_t dataLocation[]) 
{
	int32_t convertedValue;
	uint8_t highByte;
	uint8_t lowByte;
	int32_t realMinimum = min(valueMinimum, valueMaximum);
	int32_t realMaximum = max(valueMinimum, valueMaximum);

	if (includeValue == false) return 0;

	if (value < realMinimum) {
		value = realMinimum;
	}
	if (value > realMaximum) {
		value = realMaximum;
	}

	if (valueMinimum > valueMaximum) {
		// Values go from a larger number to a smaller number (e.g. 1024 to 0)
		value = realMaximum - value + realMinimum;
	}

	convertedValue = map(value, realMinimum, realMaximum, actualMinimum, actualMaximum);

	highByte = (uint8_t)(convertedValue >> 8);
	lowByte = (uint8_t)(convertedValue & 0x00FF);
	
	dataLocation[0] = lowByte;
	dataLocation[1] = highByte;
	
	return 2;
}

int Joystick_::buildAndSetAxisValue(bool includeAxis, int32_t axisValue, int32_t axisMinimum, int32_t axisMaximum, uint8_t dataLocation[]) 
{
	return buildAndSet16BitValue(includeAxis, axisValue, axisMinimum, axisMaximum, JOYSTICK_AXIS_MINIMUM, JOYSTICK_AXIS_MAXIMUM, dataLocation);
}

void Joystick_::sendState()
{
	if (_hidReportSize == 0 || _hidReportSize > 64) return;
	
    uint8_t data[_hidReportSize];
    int index = 0;
    
    uint8_t* buttonValues = _digitalInputs.getButtonValues();
    uint8_t buttonArraySize = _digitalInputs.getButtonValuesArraySize();
    for (; index < buttonArraySize; index++) {
        data[index] = buttonValues[index];
    }

    if (_hatSwitchCount > 0) {
        int16_t* hatSwitchValues = _digitalInputs.getHatSwitchValues();
        uint8_t convertedHatSwitch[JOYSTICK_HATSWITCH_COUNT_MAXIMUM];
        for (int hatSwitchIndex = 0; hatSwitchIndex < JOYSTICK_HATSWITCH_COUNT_MAXIMUM; hatSwitchIndex++) {
            if (hatSwitchValues[hatSwitchIndex] < 0) {
                convertedHatSwitch[hatSwitchIndex] = 8;
            } else {
                convertedHatSwitch[hatSwitchIndex] = (hatSwitchValues[hatSwitchIndex] % 360) / 45;
            }
        }
        data[index++] = (convertedHatSwitch[1] << 4) | (B00001111 & convertedHatSwitch[0]);
    }

    for (uint8_t i = 0; i < ANALOG_AXIS_COUNT; i++) {
        if (_analogAxes.isAxisEnabled(i)) {
            index += buildAndSetAxisValue(true, 
                                         _analogAxes.getAxisValue(i), 
                                         _analogAxes.getAxisMinimum(i), 
                                         _analogAxes.getAxisMaximum(i), 
                                         &(data[index]));
        }
    }

    DynamicHID().SendReport(_hidReportId, data, _hidReportSize);
}

#endif
