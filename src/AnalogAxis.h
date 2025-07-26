#ifndef ANALOGAXIS_h
#define ANALOGAXIS_h

#include <stdint.h>
#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "AxisProcessing.h"

// ADS1115 channel definitions
#define ADS1115_CH0 100
#define ADS1115_CH1 101
#define ADS1115_CH2 102
#define ADS1115_CH3 103

#define ANALOG_AXIS_COUNT 8 

// Forward declarations - actual implementations are in AxisProcessing.h
// This keeps the interface clean while the processing logic is modularized

class AnalogAxisManager {
private:
    // Axis ranges and calibration
    int16_t _axisMinimum[ANALOG_AXIS_COUNT];
    int16_t _axisMaximum[ANALOG_AXIS_COUNT];
    int16_t _axisCalibMin[ANALOG_AXIS_COUNT];
    int16_t _axisCalibMax[ANALOG_AXIS_COUNT];
    
    // Current axis values
    int16_t _axisValues[ANALOG_AXIS_COUNT];
    
    // Per-axis processing
    AxisFilter _filters[ANALOG_AXIS_COUNT];
    AxisCurve _curves[ANALOG_AXIS_COUNT];
    AxisDeadband _deadbands[ANALOG_AXIS_COUNT];
    
    // Axis enable flags
    uint8_t _enabledAxes = 0;

    // Pin assignments
    int8_t _axisPins[ANALOG_AXIS_COUNT];  // -1 = not assigned

public:
    enum AxisIndex { 
        AXIS_X = 0, AXIS_Y, AXIS_Z, AXIS_RX, AXIS_RY, AXIS_RZ,
        AXIS_S1, AXIS_S2
    };
    
    AnalogAxisManager();
    
    // Axis configuration
    void enableAxis(uint8_t axis, bool enable);
    void setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum);
    void setAxisCalibration(uint8_t axis, int32_t calibMin, int32_t calibMax);
    
    // Filtering and curves
    void setAxisFilterLevel(uint8_t axis, AxisFilterLevel level);
    void setAxisNoiseThreshold(uint8_t axis, int32_t threshold);
    void setAxisVelocityThreshold(uint8_t axis, int32_t threshold);
    void setAxisEwmaAlpha(uint8_t axis, uint32_t alphaValue);
    void setAxisResponseCurve(uint8_t axis, ResponseCurveType type);
    void setAxisCustomCurve(uint8_t axis, const int16_t* table, uint8_t points);
    
    // Deadband configuration
    void setAxisDeadbandSize(uint8_t axis, int32_t size);
    
    // Pin assignment
    void setAxisPin(uint8_t axis, int8_t pin);
    int8_t getAxisPin(uint8_t axis);
    
    // Value processing
    int32_t processAxisValue(uint8_t axis, int32_t rawValue);
    int32_t getAxisValue(uint8_t axis);
    
    // Read raw values from pins
    void readAllAxes();
    int32_t readAxisRaw(uint8_t axis);
    
    // Getters for joystick integration
    uint8_t getEnabledAxes() { return _enabledAxes; }
    uint8_t getAxisCount();
    int32_t getAxisMinimum(uint8_t axis);
    int32_t getAxisMaximum(uint8_t axis);

    // Add missing method for unified axis checking
    bool isAxisEnabled(uint8_t axis) { 
        return (axis < ANALOG_AXIS_COUNT) && (_enabledAxes & (1 << axis)); 
    }
};

// Function to initialize ADS1115 if needed
void initializeADS1115IfNeeded();

// Functions for round-robin ADS1115 reading to prevent encoder lag
void registerADS1115Channel(uint8_t channel);
void performRoundRobinADS1115Read();

#endif // ANALOGAXIS_h
