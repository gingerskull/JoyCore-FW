// SPDX-License-Identifier: GPL-3.0-or-later
#include "AxisProcessing.h"

// =============================================================================
// AXIS FILTER IMPLEMENTATION
// =============================================================================

void AxisFilter::reset() {
    filteredValue = 0;
    lastRawValue = 0;
    lastProcessedValue = 0;
    lastUpdateTime = 0;
    initialized = false;
}

int32_t AxisFilter::filter(int32_t rawValue) {
    // If filtering is disabled, pass through raw value
    if (filterLevel == AXIS_FILTER_OFF) return rawValue;
    
    uint32_t currentTime = millis();
    
    // Initialize filter on first run
    if (!initialized) {
        filteredValue = rawValue;
        lastRawValue = rawValue;
        lastProcessedValue = rawValue;
        lastUpdateTime = currentTime;
        initialized = true;
        return rawValue;
    }
    
    // Calculate change metrics
    int32_t deltaValue = abs(rawValue - lastProcessedValue);
    uint32_t deltaTime = currentTime - lastUpdateTime;
    if (deltaTime == 0) deltaTime = 1; // Prevent division by zero
    
    // Calculate velocity (change per unit time * 100 for scaling)
    int32_t velocity = (deltaValue * 100) / deltaTime;
    
    // If change is below noise threshold and velocity is low, return cached value
    if (deltaValue < noiseThreshold && velocity < velocityThreshold) {
        lastUpdateTime = currentTime;
        return filteredValue;
    }
    
    // Adaptive smoothing: reduce smoothing factor during fast movements
    uint8_t adaptiveSmoothingFactor = smoothingFactor;
    if (velocity > velocityThreshold) {
        adaptiveSmoothingFactor = max(0, smoothingFactor - 2);
    }
    
    // Emergency pass-through for very fast movements or large jumps
    if (velocity > velocityThreshold * 3 || deltaValue > 100) {
        filteredValue = rawValue;
    } else if (adaptiveSmoothingFactor == 0) {
        // No smoothing
        filteredValue = rawValue;
    } else {
        // Apply exponential smoothing: filtered += (raw - filtered) >> smoothingFactor
        int32_t delta = rawValue - filteredValue;
        filteredValue += (delta >> adaptiveSmoothingFactor);
    }
    
    // Update state for next iteration
    lastRawValue = rawValue;
    lastProcessedValue = rawValue;
    lastUpdateTime = currentTime;
    
    return filteredValue;
}

void AxisFilter::setLevel(AxisFilterLevel level) {
    filterLevel = level;
    
    // Set predefined parameters for each filter level
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
    
    // Reset filter state when level changes
    reset();
}

void AxisFilter::setNoiseThreshold(int32_t threshold) {
    noiseThreshold = threshold;
}

void AxisFilter::setSmoothingFactor(uint8_t factor) {
    // Clamp to valid range (0-7)
    if (factor <= 7) {
        smoothingFactor = factor;
    }
}

void AxisFilter::setVelocityThreshold(int32_t threshold) {
    velocityThreshold = threshold;
}

// =============================================================================
// AXIS CURVE IMPLEMENTATION
// =============================================================================

int32_t AxisCurve::apply(int32_t input) {
    // Linear curve is a simple pass-through
    if (type == CURVE_LINEAR) {
        return input;
    }
    
    // Select the appropriate curve table
    const int32_t* table;
    
    switch (type) {
        case CURVE_LINEAR:
            table = PRESET_CURVES[0];
            break;
        case CURVE_S_CURVE:
            table = PRESET_CURVES[1];
            break;
        case CURVE_EXPONENTIAL:
            table = PRESET_CURVES[2];
            break;
        case CURVE_CUSTOM:
            table = customTable;
            break;
        default:
            table = PRESET_CURVES[0]; // Default to linear
            break;
    }
    
    // Perform linear interpolation between curve points
    int32_t maxInput = 1023; // Expected input range
    int32_t idx = (input * (points - 1)) / maxInput;
    
    // Clamp to valid index range
    if (idx >= points - 1) return table[points - 1];
    if (idx < 0) return table[0];
    
    // Calculate interpolation points
    int32_t x0 = (idx * maxInput) / (points - 1);
    int32_t x1 = ((idx + 1) * maxInput) / (points - 1);
    int32_t y0 = table[idx];
    int32_t y1 = table[idx + 1];
    
    // Handle edge case where x0 == x1
    if (x1 == x0) return y0;
    
    // Linear interpolation: y = y0 + (input - x0) * (y1 - y0) / (x1 - x0)
    return y0 + (input - x0) * (y1 - y0) / (x1 - x0);
}

void AxisCurve::setType(ResponseCurveType newType) {
    type = newType;
}

void AxisCurve::setCustomCurve(const int32_t* newTable, uint8_t newPoints) {
    // Validate input parameters
    if (newPoints > 1 && newPoints <= 11 && newTable != nullptr) {
        // Copy the new curve points
        for (uint8_t i = 0; i < newPoints; ++i) {
            customTable[i] = newTable[i];
        }
        points = newPoints;
        type = CURVE_CUSTOM;
    }
} 