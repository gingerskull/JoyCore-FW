#ifndef ANALOGAXIS_h
#define ANALOGAXIS_h

#include <stdint.h>
#include <Arduino.h>
#include <Adafruit_ADS1X15.h>

// ADS1115 channel definitions
#define ADS1115_CH0 100
#define ADS1115_CH1 101
#define ADS1115_CH2 102
#define ADS1115_CH3 103

enum ResponseCurveType {
    CURVE_LINEAR,
    CURVE_S_CURVE,
    CURVE_EXPONENTIAL,
    CURVE_CUSTOM
};

enum AxisFilterLevel {
    AXIS_FILTER_OFF,
    AXIS_FILTER_LOW,
    AXIS_FILTER_MEDIUM,
    AXIS_FILTER_HIGH
};

#define ANALOG_AXIS_COUNT 8 // X, Y, Z, Rx, Ry, Rz, S1, S2

struct AxisFilter {
    int32_t filteredValue = 0;
    int32_t lastRawValue = 0;
    int32_t noiseThreshold = 2;
    uint8_t smoothingFactor = 3;
    bool initialized = false;
    AxisFilterLevel filterLevel = AXIS_FILTER_MEDIUM;
    
    int32_t velocityThreshold = 20;
    int32_t lastProcessedValue = 0;
    uint32_t lastUpdateTime = 0;

    void reset();
    int32_t filter(int32_t rawValue);
    void setLevel(AxisFilterLevel level);
    void setNoiseThreshold(int32_t threshold);
    void setSmoothingFactor(uint8_t factor);
    void setVelocityThreshold(int32_t threshold);
};

struct AxisCurve {
    ResponseCurveType type = CURVE_LINEAR;
    int32_t customTable[11] = {0, 102, 204, 306, 408, 512, 614, 716, 818, 920, 1023};
    uint8_t points = 11;

    int32_t apply(int32_t input);
    void setType(ResponseCurveType newType);
    void setCustomCurve(const int32_t* newTable, uint8_t newPoints);
};

class AnalogAxisManager {
private:
    // Axis ranges and calibration
    int32_t _axisMinimum[ANALOG_AXIS_COUNT];
    int32_t _axisMaximum[ANALOG_AXIS_COUNT];
    int32_t _axisCalibMin[ANALOG_AXIS_COUNT];
    int32_t _axisCalibMax[ANALOG_AXIS_COUNT];
    
    // Current axis values
    int32_t _axisValues[ANALOG_AXIS_COUNT];
    
    // Per-axis processing
    AxisFilter _filters[ANALOG_AXIS_COUNT];
    AxisCurve _curves[ANALOG_AXIS_COUNT];
    
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
    void setAxisSmoothingFactor(uint8_t axis, uint8_t factor);
    void setAxisVelocityThreshold(uint8_t axis, int32_t threshold);
    void setAxisResponseCurve(uint8_t axis, ResponseCurveType type);
    void setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points);
    
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

static constexpr int32_t PRESET_TABLES[3][11] = {
  {0,102,204,306,408,512,614,716,818,920,1023},  // linear
  {0,10,40,120,260,512,764,904,984,1013,1023},  // s-curve
  {0,5,20,45,80,125,180,245,320,405,1023}       // expo
};

#endif // ANALOGAXIS_h
