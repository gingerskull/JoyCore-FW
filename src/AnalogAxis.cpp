#include "AnalogAxis.h"

// ADS1115 instance and initialization flag
Adafruit_ADS1115 ads;
bool adsInitialized = false;

// ADS1115 rate limiting variables (moved outside function to avoid static variable issues)
static int32_t adsLastValues[4] = {0, 0, 0, 0};
static unsigned long adsLastReadTimes[4] = {0, 0, 0, 0};

// Round-robin ADS1115 reading to prevent encoder lag
static uint8_t adsRoundRobinIndex = 0;
static uint8_t adsChannelsInUse[4] = {255, 255, 255, 255}; // 255 = not in use
static uint8_t adsChannelCount = 0;
static unsigned long adsRoundRobinInterval = 20; // Read one channel every 20ms

// Note: AxisFilter and AxisCurve implementations have been moved to AxisProcessing.cpp
// This file now focuses on AnalogAxisManager and hardware interface

// AnalogAxisManager implementation
AnalogAxisManager::AnalogAxisManager() {
    for (uint8_t i = 0; i < ANALOG_AXIS_COUNT; i++) {
        _axisMinimum[i] = 0;
        _axisMaximum[i] = 1023;
        _axisCalibMin[i] = 0;
        _axisCalibMax[i] = 1023;
        _axisValues[i] = 0;
        _axisPins[i] = -1;  // No pin assigned by default
    }
    _enabledAxes = 0;
}

void AnalogAxisManager::enableAxis(uint8_t axis, bool enable) {
    if (axis < ANALOG_AXIS_COUNT) {
        if (enable) {
            _enabledAxes |= (1 << axis);
        } else {
            _enabledAxes &= ~(1 << axis);
        }
    }
}

void AnalogAxisManager::setAxisRange(uint8_t axis, int32_t minimum, int32_t maximum) {
    if (axis < ANALOG_AXIS_COUNT) {
        _axisMinimum[axis] = minimum;
        _axisMaximum[axis] = maximum;
    }
}

void AnalogAxisManager::setAxisCalibration(uint8_t axis, int32_t calibMin, int32_t calibMax) {
    if (axis < ANALOG_AXIS_COUNT) {
        _axisCalibMin[axis] = calibMin;
        _axisCalibMax[axis] = calibMax;
    }
}

void AnalogAxisManager::setAxisFilterLevel(uint8_t axis, AxisFilterLevel level) {
    if (axis < ANALOG_AXIS_COUNT) _filters[axis].setLevel(level);
}

void AnalogAxisManager::setAxisNoiseThreshold(uint8_t axis, int32_t threshold) {
    if (axis < ANALOG_AXIS_COUNT) _filters[axis].setNoiseThreshold(threshold);
}

void AnalogAxisManager::setAxisSmoothingFactor(uint8_t axis, uint8_t factor) {
    if (axis < ANALOG_AXIS_COUNT) _filters[axis].setSmoothingFactor(factor);
}

void AnalogAxisManager::setAxisVelocityThreshold(uint8_t axis, int32_t threshold) {
    if (axis < ANALOG_AXIS_COUNT) _filters[axis].setVelocityThreshold(threshold);
}

void AnalogAxisManager::setAxisEwmaAlpha(uint8_t axis, uint32_t alphaValue) {
    if (axis < ANALOG_AXIS_COUNT) _filters[axis].setEwmaAlpha(alphaValue);
}

void AnalogAxisManager::setAxisResponseCurve(uint8_t axis, ResponseCurveType type) {
    if (axis < ANALOG_AXIS_COUNT) _curves[axis].setType(type);
}

void AnalogAxisManager::setAxisCustomCurve(uint8_t axis, const int16_t* table, uint8_t points) {
    if (axis < ANALOG_AXIS_COUNT) _curves[axis].setCustomCurve(table, points);
}

void AnalogAxisManager::setAxisDeadbandSize(uint8_t axis, int32_t size) {
    if (axis < ANALOG_AXIS_COUNT) {
        _deadbands[axis].setSize(size);
    }
}

int32_t AnalogAxisManager::processAxisValue(uint8_t axis, int32_t rawValue) {
    if (axis >= ANALOG_AXIS_COUNT) return rawValue;
    
    // First map raw hardware value to user-defined range
    int32_t sourceMin, sourceMax;
    int8_t pin = _axisPins[axis];
    
    if (pin >= 100 && pin <= 103) {
        // ADS1115 channels: 16-bit range (0-16383)
        sourceMin = 0;
        sourceMax = 16383;
    } else {
        // Analog pins: 10-bit range (0-1023) on RP2040
        sourceMin = 0;
        sourceMax = 1023;
    }
    
    // Map from hardware range to user-defined range
    int32_t mappedValue = map(rawValue, sourceMin, sourceMax, _axisMinimum[axis], _axisMaximum[axis]);
    mappedValue = constrain(mappedValue, _axisMinimum[axis], _axisMaximum[axis]);
    
    // Apply deadband FIRST on the raw mapped signal, then filtering and curves
    int32_t deadbanded = _deadbands[axis].apply(mappedValue);
    int32_t filtered = _filters[axis].filter(deadbanded);
    int32_t curved = _curves[axis].apply(filtered);
    
    // Map to joystick range (-32767 to 32767)
    int32_t finalValue = map(curved, _axisMinimum[axis], _axisMaximum[axis], -32767, 32767);
    
    _axisValues[axis] = finalValue;
    return finalValue;
}

int32_t AnalogAxisManager::getAxisValue(uint8_t axis) {
    if (axis < ANALOG_AXIS_COUNT) {
        return _axisValues[axis];
    }
    return 0;
}

uint8_t AnalogAxisManager::getAxisCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < ANALOG_AXIS_COUNT; i++) {
        if (_enabledAxes & (1 << i)) count++;
    }
    return count;
}

int32_t AnalogAxisManager::getAxisMinimum(uint8_t axis) {
    if (axis < ANALOG_AXIS_COUNT) return _axisMinimum[axis];
    return 0;
}

int32_t AnalogAxisManager::getAxisMaximum(uint8_t axis) {
    if (axis < ANALOG_AXIS_COUNT) return _axisMaximum[axis];
    return 1023;
}

void AnalogAxisManager::setAxisPin(uint8_t axis, int8_t pin) {
    if (axis < ANALOG_AXIS_COUNT) {
        _axisPins[axis] = pin;
        
        // Auto-register ADS1115 channels for round-robin reading
        if (pin >= 100 && pin <= 103) {
            registerADS1115Channel(pin - 100);
        }
    }
}

int8_t AnalogAxisManager::getAxisPin(uint8_t axis) {
    if (axis < ANALOG_AXIS_COUNT) {
        return _axisPins[axis];
    }
    return -1;
}

int32_t AnalogAxisManager::readAxisRaw(uint8_t axis) {
    if (axis < ANALOG_AXIS_COUNT && _axisPins[axis] >= 0) {
        int8_t pin = _axisPins[axis];
        if (pin >= 100 && pin <= 103) { // ADS1115 channels
            if (adsInitialized) {
                uint8_t channel = pin - 100;
                // Return last known value for ADS1115 channels
                // Round-robin reading is handled separately
                return adsLastValues[channel];
            }
            return 0;
        } else {
            return analogRead(pin);
        }
    }
    return 0;
}

void AnalogAxisManager::readAllAxes() {
    static unsigned long lastReadTime = 0;
    unsigned long currentTime = millis();
    
    // Enforce consistent timing regardless of input source
    // This ensures EWMA filtering behaves consistently
    if (currentTime - lastReadTime < 5) {
        return; // Skip this read cycle to maintain consistent timing
    }
    lastReadTime = currentTime;
    
    // Perform round-robin ADS1115 reading to prevent encoder lag
    performRoundRobinADS1115Read();
    
    // Read all axes (ADS1115 channels return cached values, analog pins read directly)
    for (uint8_t i = 0; i < ANALOG_AXIS_COUNT; i++) {
        if (isAxisEnabled(i) && _axisPins[i] >= 0) {
            int32_t rawValue = readAxisRaw(i);
            processAxisValue(i, rawValue);
        }
    }
}

void initializeADS1115IfNeeded() {
    if (!adsInitialized) {
        ads.begin();
        adsInitialized = true;
    }
}

void registerADS1115Channel(uint8_t channel) {
    if (channel > 3) return;
    
    // Check if channel is already registered
    for (uint8_t i = 0; i < adsChannelCount; i++) {
        if (adsChannelsInUse[i] == channel) return;
    }
    
    // Add channel to the list if there's space
    if (adsChannelCount < 4) {
        adsChannelsInUse[adsChannelCount] = channel;
        adsChannelCount++;
        // Initialize with a reasonable center value
        adsLastValues[channel] = 8192; // Mid-range for 16-bit ADC
    }
}

void performRoundRobinADS1115Read() {
    if (!adsInitialized || adsChannelCount == 0) return;
    
    static unsigned long lastRoundRobinTime = 0;
    unsigned long currentTime = millis();
    
    // Only read one channel per interval to prevent blocking
    if (currentTime - lastRoundRobinTime >= adsRoundRobinInterval) {
        uint8_t channel = adsChannelsInUse[adsRoundRobinIndex];
        
        // Read the current channel
        int16_t val = ads.readADC_SingleEnded(channel);
        if (val >= 0) { // Valid reading
            adsLastValues[channel] = val;
            adsLastReadTimes[channel] = currentTime;
        }
        
        // Move to next channel for next iteration
        adsRoundRobinIndex = (adsRoundRobinIndex + 1) % adsChannelCount;
        lastRoundRobinTime = currentTime;
    }
}