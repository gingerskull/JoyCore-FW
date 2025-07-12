#ifndef DIGITALINPUTS_h
#define DIGITALINPUTS_h

#include <stdint.h>
#include <Arduino.h>

#define DIGITAL_INPUTS_MAX_BUTTONS 32
#define DIGITAL_INPUTS_MAX_HAT_SWITCHES 2
#define DIGITAL_INPUTS_HATSWITCH_RELEASE -1

class DigitalInputsManager {
private:
    uint8_t _buttonCount;
    uint8_t _hatSwitchCount;
    uint8_t _buttonValuesArraySize;
    uint8_t* _buttonValues;
    int16_t _hatSwitchValues[DIGITAL_INPUTS_MAX_HAT_SWITCHES];

public:
    DigitalInputsManager(uint8_t buttonCount = DIGITAL_INPUTS_MAX_BUTTONS, 
                        uint8_t hatSwitchCount = 2);
    ~DigitalInputsManager();
    
    // Button methods
    void setButton(uint8_t button, uint8_t value);
    void pressButton(uint8_t button);
    void releaseButton(uint8_t button);
    uint8_t getButton(uint8_t button);
    
    // Hat switch methods
    void setHatSwitch(uint8_t hatSwitch, int16_t value);
    int16_t getHatSwitch(uint8_t hatSwitch);
    
    // Configuration getters
    uint8_t getButtonCount() { return _buttonCount; }
    uint8_t getHatSwitchCount() { return _hatSwitchCount; }
    uint8_t getButtonValuesArraySize() { return _buttonValuesArraySize; }
    uint8_t* getButtonValues() { return _buttonValues; }
    int16_t* getHatSwitchValues() { return _hatSwitchValues; }
    
    // Utility
    void reset();
};

#endif
