// SPDX-License-Identifier: GPL-3.0-or-later
#include "AxisProcessing.h"

// =============================================================================
// AXIS FILTER IMPLEMENTATION
// =============================================================================

void AxisFilter::reset() {
    initialized = false;
    ewmaFilter.reset();
}

int32_t AxisFilter::filter(int32_t rawValue) {
    // If filtering is disabled, pass through raw value
    if (filterLevel == AXIS_FILTER_OFF) return rawValue;
    
    // Handle EWMA filtering
    if (filterLevel == AXIS_FILTER_EWMA) {
        return ewmaFilter.filter(rawValue);
    }
    
    // Default fallback (should not be reached)
    return rawValue;
}

void AxisFilter::setLevel(AxisFilterLevel level) {
    filterLevel = level;
    
    // Set parameters based on filter level
    switch (level) {
        case AXIS_FILTER_OFF:
            // No filtering - pass through
            break;
            
        case AXIS_FILTER_EWMA:
            ewmaFilter.setAlpha(30); // Default alpha = 0.03 (30/1000)
            break;
    }
    
    // Reset filter state when level changes
    reset();
}


void AxisFilter::setEwmaAlpha(uint32_t alphaValue) {
    ewmaFilter.setAlpha(alphaValue);
}

// =============================================================================
// AXIS CURVE IMPLEMENTATION
// =============================================================================

int32_t AxisCurve::apply(int32_t input) {
    // All curves now use custom table (EEPROM-stored configuration)
    const int32_t* table = customTable;
    
    // Perform linear interpolation between curve points
    // Use the actual input range from the user configuration (0-32767)
    int32_t maxInput = 32767; // User-defined range
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
    // Only CURVE_CUSTOM is supported now
    type = CURVE_CUSTOM;
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

// =============================================================================
// AXIS DEADBAND IMPLEMENTATION
// =============================================================================

int32_t AxisDeadband::apply(int32_t input) {
    // If deadband is disabled, pass through unchanged
    if (deadbandSize <= 0) {
        return input;
    }
    
    uint32_t currentTime = millis();
    
    // Initialize with first input value
    if (!initialized) {
        lastInput = input;
        stableValue = input;
        lastSampleTime = currentTime;
        deadbandActive = false;
        capturedStableValue = false;
        initialized = true;
        return input;
    }
    
    // Sample movement at regular intervals for statistical analysis
    if (currentTime - lastSampleTime >= (settleDuration / HISTORY_SIZE)) {
        // Calculate movement since last reading
        int32_t movement = abs(input - lastInput);
        
        // Add to movement history (ring buffer)
        movementHistory[historyIndex] = movement;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        if (historySamples < HISTORY_SIZE) {
            historySamples++;
        }
        
        lastSampleTime = currentTime;
        lastInput = input;
        
        // Only evaluate deadband state when we have sufficient samples
        if (historySamples >= HISTORY_SIZE) {
            int32_t avgMovement = getAverageMovement();
            int32_t movementThreshold = deadbandSize / 8; // More sensitive threshold
            
            if (avgMovement <= movementThreshold) {
                // Low average movement detected
                if (!capturedStableValue) {
                    // First time detecting minimal movement - capture the stable value now
                    stableValue = input;
                    capturedStableValue = true;
                }
                
                // Activate deadband if not already active
                if (!deadbandActive) {
                    deadbandActive = true;
                }
            } else {
                // High average movement - deactivate deadband and reset capture flag
                deadbandActive = false;
                capturedStableValue = false;
            }
        }
    }
    
    // Return appropriate value based on deadband state
    if (deadbandActive) {
        // Deadband is active - check if movement exceeds threshold
        if (abs(input - stableValue) > deadbandSize) {
            // Large movement - deactivate deadband immediately
            deadbandActive = false;
            capturedStableValue = false;
            stableValue = input;
            // Clear movement history to avoid lag in reactivation
            historySamples = 0;
            historyIndex = 0;
            return input;
        } else {
            // Small movement - maintain stable value
            return stableValue;
        }
    } else {
        // Deadband not active - pass through input
        return input;
    }
}

void AxisDeadband::setSize(int32_t size) {
    deadbandSize = max(0, size); // Ensure non-negative
}

void AxisDeadband::setSettleDuration(uint32_t duration) {
    settleDuration = duration;
}

void AxisDeadband::reset() {
    initialized = false;
    lastInput = 0;
    stableValue = 0;
    lastSampleTime = 0;
    deadbandActive = false;
    capturedStableValue = false;
    historyIndex = 0;
    historySamples = 0;
    for (uint8_t i = 0; i < HISTORY_SIZE; i++) {
        movementHistory[i] = 0;
    }
}

int32_t AxisDeadband::getAverageMovement() const {
    if (historySamples == 0) return 0;
    
    int32_t sum = 0;
    for (uint8_t i = 0; i < historySamples; i++) {
        sum += movementHistory[i];
    }
    return sum / historySamples;
}

// =============================================================================
// EWMA FILTER IMPLEMENTATION
// =============================================================================

void EwmaFilter::reset() {
    lastOutput = 0;
    initialized = false;
}

int32_t EwmaFilter::filter(int32_t input) {
    // Initialize filter on first run
    if (!initialized) {
        lastOutput = input;
        initialized = true;
        return input;
    }
    
    // Apply EWMA formula: output = alpha * input + (1 - alpha) * lastOutput
    // Using integer arithmetic: output = (alpha * input + (alphaScale - alpha) * lastOutput) / alphaScale
    int32_t output = (alpha * input + (alphaScale - alpha) * lastOutput) / alphaScale;
    
    lastOutput = output;
    return output;
}

void EwmaFilter::setAlpha(uint32_t alphaValue) {
    // Clamp alpha to valid range [0, alphaScale]
    if (alphaValue <= alphaScale) {
        alpha = alphaValue;
    }
    
    // Reset filter when alpha changes for clean transition
    reset();
}