#include "Buttons.h"
#include <Arduino.h>

DigitalInputsManager::DigitalInputsManager(uint8_t buttonCount, uint8_t hatSwitchCount) 
    : _buttonCount(buttonCount), _hatSwitchCount(hatSwitchCount) {
    
    // Setup button array
    if (_buttonCount > 0) {
        _buttonValuesArraySize = _buttonCount / 8;
        if ((_buttonCount % 8) > 0) {
            _buttonValuesArraySize++;
        }
        _buttonValues = new uint8_t[_buttonValuesArraySize];
    } else {
        _buttonValuesArraySize = 0;
        _buttonValues = nullptr;
    }
    
    reset();
}

DigitalInputsManager::~DigitalInputsManager() {
    if (_buttonValues) {
        delete[] _buttonValues;
    }
}

void DigitalInputsManager::setButton(uint8_t button, uint8_t value) {
    if (value == 0) {
        releaseButton(button);
    } else {
        pressButton(button);
    }
}

void DigitalInputsManager::pressButton(uint8_t button) {
    if (button >= _buttonCount || !_buttonValues) return;

    int index = button / 8;
    int bit = button % 8;
    bitSet(_buttonValues[index], bit);
}

void DigitalInputsManager::releaseButton(uint8_t button) {
    if (button >= _buttonCount || !_buttonValues) return;

    int index = button / 8;
    int bit = button % 8;
    bitClear(_buttonValues[index], bit);
}

uint8_t DigitalInputsManager::getButton(uint8_t button) {
    if (button >= _buttonCount || !_buttonValues) return 0;
    
    int index = button / 8;
    int bit = button % 8;
    return (_buttonValues[index] >> bit) & 1;
}

void DigitalInputsManager::setHatSwitch(uint8_t hatSwitch, int16_t value) {
    if (hatSwitch >= _hatSwitchCount) return;
    _hatSwitchValues[hatSwitch] = value;
}

int16_t DigitalInputsManager::getHatSwitch(uint8_t hatSwitch) {
    if (hatSwitch >= _hatSwitchCount) return DIGITAL_INPUTS_HATSWITCH_RELEASE;
    return _hatSwitchValues[hatSwitch];
}

void DigitalInputsManager::reset() {
    // Reset buttons
    if (_buttonValues) {
        for (int index = 0; index < _buttonValuesArraySize; index++) {
            _buttonValues[index] = 0;
        }
    }
    
    // Reset hat switches
    for (int index = 0; index < DIGITAL_INPUTS_MAX_HAT_SWITCHES; index++) {
        _hatSwitchValues[index] = DIGITAL_INPUTS_HATSWITCH_RELEASE;
    }
}
