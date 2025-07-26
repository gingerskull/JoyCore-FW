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
    ewmaFilter.reset();
}

int32_t AxisFilter::filter(int32_t rawValue) {
    // If filtering is disabled, pass through raw value
    if (filterLevel == AXIS_FILTER_OFF) return rawValue;
    
    // Handle EWMA filtering separately
    if (filterLevel == AXIS_FILTER_EWMA) {
        return ewmaFilter.filter(rawValue);
    }
    
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
    
    // Emergency pass-through for very fast movements or large jumps
    if (velocity > velocityThreshold * 3 || deltaValue > 100) {
        filteredValue = rawValue;
    } else {
        // Apply exponential smoothing: filtered += (raw - filtered) >> smoothingFactor
        int32_t delta = rawValue - filteredValue;
        filteredValue += (delta >> 2); // Fixed smoothing factor of 4
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
            noiseThreshold = 0; 
            velocityThreshold = 0;
            break;
            
        case AXIS_FILTER_LOW:
            noiseThreshold = 1; 
            velocityThreshold = 15;
            break;
            
        case AXIS_FILTER_MEDIUM:
            noiseThreshold = 2; 
            velocityThreshold = 20;
            break;
            
        case AXIS_FILTER_HIGH:
            noiseThreshold = 6; 
            velocityThreshold = 50;
            break;
            
        case AXIS_FILTER_EWMA:
            // EWMA uses its own algorithm, so these parameters are not used
            noiseThreshold = 0; 
            velocityThreshold = 0;
            ewmaFilter.setAlpha(30); // Default alpha = 0.03 (30/1000)
            break;
    }
    
    // Reset filter state when level changes
    reset();
}

void AxisFilter::setNoiseThreshold(int32_t threshold) {
    noiseThreshold = threshold;
}

void AxisFilter::setVelocityThreshold(int32_t threshold) {
    velocityThreshold = threshold;
}

void AxisFilter::setEwmaAlpha(uint32_t alphaValue) {
    ewmaFilter.setAlpha(alphaValue);
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
    const int16_t* table;
    
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
            table = PRESET_CURVES[3];
            break;
        default:
            table = PRESET_CURVES[0]; // Default to linear
            break;
    }
    
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
    type = newType;
}

void AxisCurve::setCustomCurve(const int16_t* newTable, uint8_t newPoints) {
    // Validate input parameters - we only support 11 points for preset curves
    if (newPoints == 11 && newTable != nullptr) {
        // Copy the new curve points to the global custom curve slot
        // Note: This modifies the global PRESET_CURVES[3] array, so all axes
        // using CURVE_CUSTOM will share the same custom curve
        for (uint8_t i = 0; i < 11; ++i) {
            // We need to cast away const to modify the array
            const_cast<int16_t*>(PRESET_CURVES[3])[i] = newTable[i];
        }
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

void AxisDeadband::setSize(int16_t size) {
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