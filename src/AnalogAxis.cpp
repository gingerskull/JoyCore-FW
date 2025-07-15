#include "AnalogAxis.h"
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
bool adsInitialized = false;

// AxisFilter implementation
void AxisFilter::reset() {
    filteredValue = 0;
    lastRawValue = 0;
    lastProcessedValue = 0;
    lastUpdateTime = 0;
    initialized = false;
}

int32_t AxisFilter::filter(int32_t rawValue) {
    if (filterLevel == AXIS_FILTER_OFF) return rawValue;
    
    uint32_t currentTime = millis();
    
    if (!initialized) {
        filteredValue = rawValue;
        lastRawValue = rawValue;
        lastProcessedValue = rawValue;
        lastUpdateTime = currentTime;
        initialized = true;
        return rawValue;
    }
    
    int32_t deltaValue = abs(rawValue - lastProcessedValue);
    uint32_t deltaTime = currentTime - lastUpdateTime;
    if (deltaTime == 0) deltaTime = 1;
    
    int32_t velocity = (deltaValue * 100) / deltaTime;
    
    if (deltaValue < noiseThreshold && velocity < velocityThreshold) {
        lastUpdateTime = currentTime;
        return filteredValue;
    }
    
    uint8_t adaptiveSmoothingFactor = smoothingFactor;
    if (velocity > velocityThreshold) {
        adaptiveSmoothingFactor = max(0, smoothingFactor - 2);
    }
    
    if (velocity > velocityThreshold * 3 || deltaValue > 100) {
        filteredValue = rawValue;
    } else if (adaptiveSmoothingFactor == 0) {
        filteredValue = rawValue;
    } else {
        int32_t delta = rawValue - filteredValue;
        filteredValue += (delta >> adaptiveSmoothingFactor);
    }
    
    lastRawValue = rawValue;
    lastProcessedValue = rawValue;
    lastUpdateTime = currentTime;
    return filteredValue;
}

void AxisFilter::setLevel(AxisFilterLevel level) {
    filterLevel = level;
    switch (level) {
        case AXIS_FILTER_OFF:    
            smoothingFactor = 0; 
            noiseThreshold = 0; 
            velocityThreshold = 0;
            break;
        case AXIS_FILTER_LOW:    
            smoothingFactor = 1; 
            noiseThreshold = 1; 
            velocityThreshold = 15;
            break;
        case AXIS_FILTER_MEDIUM: 
            smoothingFactor = 3; 
            noiseThreshold = 2; 
            velocityThreshold = 20;
            break;
        case AXIS_FILTER_HIGH:   
            smoothingFactor = 4; 
            noiseThreshold = 3; 
            velocityThreshold = 25;
            break;
    }
    reset();
}

void AxisFilter::setNoiseThreshold(int32_t threshold) {
    noiseThreshold = threshold;
}

void AxisFilter::setSmoothingFactor(uint8_t factor) {
    if (factor <= 7) {
        smoothingFactor = factor;
    }
}

void AxisFilter::setVelocityThreshold(int32_t threshold) {
    velocityThreshold = threshold;
}

// AxisCurve implementation
int32_t AxisCurve::apply(int32_t input) {
    if (type == CURVE_LINEAR) {
        return input;
    }
    
    int32_t* table = customTable;
    int32_t presetLinear[11] = {0, 102, 204, 306, 408, 512, 614, 716, 818, 920, 1023};
    int32_t presetSCurve[11] = {0, 10, 40, 120, 260, 512, 764, 904, 984, 1013, 1023};
    int32_t presetExponential[11] = {0, 5, 20, 45, 80, 125, 180, 245, 320, 405, 1023};
    
    switch (type) {
        case CURVE_LINEAR: table = presetLinear; break;
        case CURVE_S_CURVE: table = presetSCurve; break;
        case CURVE_EXPONENTIAL: table = presetExponential; break;
        case CURVE_CUSTOM: table = customTable; break;
    }
    
    int32_t maxInput = 1023;
    int32_t idx = (input * (points - 1)) / maxInput;
    if (idx >= points - 1) return table[points - 1];
    int32_t x0 = (idx * maxInput) / (points - 1);
    int32_t x1 = ((idx + 1) * maxInput) / (points - 1);
    int32_t y0 = table[idx];
    int32_t y1 = table[idx + 1];
    if (x1 == x0) return y0;
    return y0 + (input - x0) * (y1 - y0) / (x1 - x0);
}

void AxisCurve::setType(ResponseCurveType newType) {
    type = newType;
}

void AxisCurve::setCustomCurve(const int32_t* newTable, uint8_t newPoints) {
    if (newPoints > 1 && newPoints <= 11) {
        for (uint8_t i = 0; i < newPoints; ++i) customTable[i] = newTable[i];
        points = newPoints;
        type = CURVE_CUSTOM;
    }
}

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

void AnalogAxisManager::setAxisResponseCurve(uint8_t axis, ResponseCurveType type) {
    if (axis < ANALOG_AXIS_COUNT) _curves[axis].setType(type);
}

void AnalogAxisManager::setAxisCustomCurve(uint8_t axis, const int32_t* table, uint8_t points) {
    if (axis < ANALOG_AXIS_COUNT) _curves[axis].setCustomCurve(table, points);
}

int32_t AnalogAxisManager::processAxisValue(uint8_t axis, int32_t rawValue) {
    if (axis >= ANALOG_AXIS_COUNT) return rawValue;
    
    int32_t filtered = _filters[axis].filter(rawValue);
    int32_t curved = _curves[axis].apply(filtered);
    _axisValues[axis] = curved;
    return curved;
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
                // Per-axis static variables to avoid conflicts
                static int32_t lastValues[4] = {0, 0, 0, 0};
                static unsigned long lastReadTimes[4] = {0, 0, 0, 0};
                uint8_t channel = pin - 100;
                unsigned long currentTime = millis();
                
                // Rate limit ADS1115 reads to prevent timing issues
                if (currentTime - lastReadTimes[channel] > 5) { // Max 200 Hz per channel
                    int16_t val = ads.readADC_SingleEnded(channel);
                    if (val >= 0) { // Valid reading
                        lastValues[channel] = val;
                        lastReadTimes[channel] = currentTime;
                    }
                    // If val < 0, use last known good value
                }
                return lastValues[channel];
            }
            return 0;
        } else {
            return analogRead(pin);
        }
    }
    return 0;
}

void AnalogAxisManager::readAllAxes() {
    for (uint8_t i = 0; i < ANALOG_AXIS_COUNT; i++) {
        if (isAxisEnabled(i) && _axisPins[i] >= 0) {
            int32_t rawValue = readAxisRaw(i);  // Use readAxisRaw instead of analogRead
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