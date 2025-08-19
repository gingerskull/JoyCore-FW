// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef AXIS_PROCESSING_H
#define AXIS_PROCESSING_H

#include <stdint.h>
#include <Arduino.h>

/**
 * @file AxisProcessing.h
 * @brief Analog axis signal processing library for joystick controllers
 *
 * This library provides signal processing capabilities for analog axes including:
 * - EWMA (Exponentially Weighted Moving Average) filtering
 * - Deadband filtering to eliminate jitter at rest
 * - Custom response curve shaping
 *
 * The processing components can be used independently:
 * - EwmaFilter: Smooth filtering with configurable responsiveness
 * - AxisDeadband: Eliminates small fluctuations when control is at rest
 * - AxisCurve: Applies custom response curves to axis values
 */

// =============================================================================
// ENUMS AND CONSTANTS
// =============================================================================

/**
 * @brief Response curve types for axis shaping
 */
enum ResponseCurveType {
    CURVE_CUSTOM       ///< User-defined custom curve (stored in EEPROM)
};

/**
 * @brief Filter levels for axis processing
 */
enum AxisFilterLevel {
    AXIS_FILTER_OFF,    ///< No filtering (raw values pass through)
    AXIS_FILTER_EWMA    ///< EWMA (Exponentially Weighted Moving Average) filtering
};

// =============================================================================
// EWMA FILTER CLASS
// =============================================================================

/**
 * @brief Exponentially Weighted Moving Average filter
 * 
 * EWMA filter provides smooth, responsive filtering with significantly less memory
 * usage compared to traditional moving average filters. Based on jonnieZG/EWMA.
 * 
 * Formula: output = alpha * input + (1 - alpha) * lastOutput
 * 
 * Features:
 * - Memory efficient (no history buffer required)
 * - Configurable alpha parameter for smoothing control
 * - Integer-only arithmetic to avoid floating point operations
 * - Automatic initialization on first reading
 */
class EwmaFilter {
private:
    int32_t lastOutput = 0;        ///< Previous filtered output value
    uint32_t alpha = 30;           ///< Smoothing factor scaled by 1000 (0-1000 range)
    uint32_t alphaScale = 1000;    ///< Scale factor for integer arithmetic
    bool initialized = false;      ///< Whether filter has received first value
    
public:
    /**
     * @brief Constructor with alpha parameter
     * @param alphaValue Alpha value scaled by 1000 (e.g., 100 = 0.1 alpha)
     */
    EwmaFilter(uint32_t alphaValue = 30) : alpha(alphaValue) {}
    
    /**
     * @brief Reset filter to initial state
     */
    void reset();
    
    /**
     * @brief Filter input value using EWMA algorithm
     * @param input Raw input value
     * @return Filtered output value
     */
    int32_t filter(int32_t input);
    
    /**
     * @brief Set alpha smoothing factor
     * @param alphaValue Alpha value scaled by 1000 (0-1000 range)
     * 
     * Higher values = less smoothing (more responsive)
     * Lower values = more smoothing (less responsive)
     * 
     * Common values:
     * - 100 (0.1) = Heavy smoothing, approximately averages last 10 readings
     * - 200 (0.2) = Moderate smoothing, approximately averages last 5 readings  
     * - 500 (0.5) = Light smoothing, approximately averages last 2 readings
     */
    void setAlpha(uint32_t alphaValue);
    
    /**
     * @brief Get current alpha value
     * @return Alpha value scaled by 1000
     */
    uint32_t getAlpha() const { return alpha; }
    
    /**
     * @brief Get last filtered output
     * @return Last output value
     */
    int32_t getOutput() const { return lastOutput; }
};

// =============================================================================
// AXIS FILTER CLASS
// =============================================================================

/**
 * @brief Filtering for analog axis values
 *
 * This class provides simple filtering for analog axis values:
 * - No filtering (raw values pass through)
 * - EWMA (Exponentially Weighted Moving Average) filtering
 *
 * The filter can be configured to use EWMA filtering for smoothing
 * or bypass filtering entirely for maximum responsiveness.
 */
class AxisFilter {
private:
    bool initialized = false;         ///< Whether filter has been initialized
    
    // Filter parameters
    AxisFilterLevel filterLevel = AXIS_FILTER_EWMA; ///< Current filter level
    
    // EWMA filter instance
    EwmaFilter ewmaFilter;            ///< EWMA filter for when AXIS_FILTER_EWMA is selected

public:
    /**
     * @brief Reset filter to initial state
     */
    void reset();
    
    /**
     * @brief Process a raw axis value through the filter
     * @param rawValue Raw input value from ADC
     * @return Filtered and smoothed output value
     */
    int32_t filter(int32_t rawValue);
    
    /**
     * @brief Set filter level
     * @param level Filter level (OFF or EWMA)
     */
    void setLevel(AxisFilterLevel level);
    
    
    /**
     * @brief Set EWMA alpha parameter
     * @param alphaValue Alpha value scaled by 1000 (0-1000 range)
     * 
     * Only applies when filter level is set to AXIS_FILTER_EWMA
     */
    void setEwmaAlpha(uint32_t alphaValue);
    
    // Getters for current settings
    AxisFilterLevel getFilterLevel() const { return filterLevel; }
    uint32_t getEwmaAlpha() const { return ewmaFilter.getAlpha(); }
};

// =============================================================================
// AXIS DEADBAND CLASS
// =============================================================================

/**
 * @brief Deadband filter for analog axes
 * 
 * Prevents small fluctuations around the current axis position when the user
 * stops moving the control. This eliminates jitter and provides stability:
 * - Uses rolling average of movement to detect settled state
 * - Activates deadband only when average movement is consistently low
 * - Maintains smooth movement during active control
 * - Stabilizes position when control is at rest
 * - Compatible with EWMA and other filtering
 * 
 * The deadband uses statistical analysis to avoid interfering with slow movements.
 */
class AxisDeadband {
private:
    static constexpr uint8_t HISTORY_SIZE = 10;  ///< Number of samples for movement analysis
    
    int32_t deadbandSize = 0;         ///< Size of deadband (0 = disabled)
    int32_t lastInput = 0;            ///< Previous input value
    int32_t stableValue = 0;          ///< Value to hold when deadband is active
    uint32_t settleDuration = 150;    ///< Time to wait before activating deadband (ms)
    bool deadbandActive = false;      ///< Whether deadband is currently active
    bool initialized = false;         ///< Whether filter has been initialized
    
    // Movement history for statistical analysis
    int32_t movementHistory[HISTORY_SIZE] = {0}; ///< Ring buffer of recent movements
    uint8_t historyIndex = 0;         ///< Current position in ring buffer
    uint8_t historySamples = 0;       ///< Number of samples collected
    uint32_t lastSampleTime = 0;      ///< Time of last sample
    
    // Stable value capture
    bool capturedStableValue = false; ///< Whether we've captured the stable value for this settle period
    
public:
    /**
     * @brief Constructor with deadband size
     * @param size Size of deadband zone (0 = disabled)
     */
    AxisDeadband(int32_t size = 0) : deadbandSize(size) {}
    
    /**
     * @brief Apply deadband to input value
     * @param input Input value
     * @return Output value with deadband applied
     */
    int32_t apply(int32_t input);
    
    /**
     * @brief Set deadband size
     * @param size Size of deadband zone (0 = disabled)
     * 
     * Typical values:
     * - 0: No deadband
     * - 500-1000: Light deadband for precision controls
     * - 1000-2000: Medium deadband for joysticks
     * - 2000-5000: Heavy deadband for worn controls
     */
    void setSize(int32_t size);
    
    /**
     * @brief Set settle duration - time to wait before activating deadband
     * @param duration Time in milliseconds (default: 150ms)
     */
    void setSettleDuration(uint32_t duration);
    
    /**
     * @brief Reset deadband state
     */
    void reset();
    
    // Getters
    int32_t getSize() const { return deadbandSize; }
    uint32_t getSettleDuration() const { return settleDuration; }
    bool isActive() const { return deadbandActive; }

private:
    /**
     * @brief Calculate average movement over recent samples
     * @return Average movement magnitude
     */
    int32_t getAverageMovement() const;
};

// =============================================================================
// AXIS CURVE CLASS
// =============================================================================

/**
 * @brief Response curve shaping for analog axis values
 *
 * This class applies custom response curves to modify the relationship between
 * input and output values. The curve is defined by a lookup table with
 * linear interpolation between points.
 */
class AxisCurve {
private:
    ResponseCurveType type = CURVE_CUSTOM; ///< Current curve type
    int32_t customTable[11] = {0, 3277, 6554, 9830, 13107, 16384, 19661, 22938, 26214, 29491, 32767}; ///< Custom curve points (linear by default, 0-32767 range)
    uint8_t points = 11; ///< Number of points in custom curve

public:
    /**
     * @brief Apply response curve to input value
     * @param input Input value (0-32767 range)
     * @return Curve-shaped output value
     */
    int32_t apply(int32_t input);
    
    /**
     * @brief Set response curve type
     * @param newType Curve type (only CUSTOM is currently supported)
     */
    void setType(ResponseCurveType newType);
    
    /**
     * @brief Define custom response curve
     * @param newTable Array of curve points (must be in ascending order)
     * @param newPoints Number of points in the curve (2-11)
     * 
     * The curve points should span the expected input range.
     * Linear interpolation is used between points.
     */
    void setCustomCurve(const int32_t* newTable, uint8_t newPoints);
    
    // Getters for current settings
    ResponseCurveType getType() const { return type; }
    uint8_t getPointCount() const { return points; }
    const int32_t* getCustomTable() const { return customTable; }
};


// =============================================================================
// HELPER FUNCTIONS
// =============================================================================


/**
 * @brief Get human-readable name for filter level
 * @param level Filter level
 * @return String representation
 */
inline const char* getFilterLevelName(AxisFilterLevel level) {
    switch (level) {
        case AXIS_FILTER_OFF:    return "Off";
        case AXIS_FILTER_EWMA:   return "EWMA";
        default:                 return "Unknown";
    }
}

/**
 * @brief Get human-readable name for curve type
 * @param type Curve type
 * @return String representation ("Custom" for CURVE_CUSTOM, "Unknown" otherwise)
 */
inline const char* getCurveTypeName(ResponseCurveType type) {
    switch (type) {
        case CURVE_CUSTOM:      return "Custom";
        default:                return "Unknown";
    }
}

#endif // AXIS_PROCESSING_H 