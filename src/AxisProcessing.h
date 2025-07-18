// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef AXIS_PROCESSING_H
#define AXIS_PROCESSING_H

#include <stdint.h>
#include <Arduino.h>

/**
 * @file AxisProcessing.h
 * @brief Analog axis signal processing library for joystick controllers
 * 
 * This library provides advanced signal processing capabilities for analog axes including:
 * - Noise filtering with configurable thresholds
 * - Adaptive smoothing based on movement velocity
 * - Response curve shaping (linear, S-curve, exponential, custom)
 * - Configurable filter levels for different use cases
 * 
 * The processing chain: Raw Input -> Noise Filter -> Velocity-Adaptive Smoothing -> Response Curve -> Output
 */

// =============================================================================
// ENUMS AND CONSTANTS
// =============================================================================

/**
 * @brief Response curve types for axis shaping
 */
enum ResponseCurveType {
    CURVE_LINEAR,      ///< Linear 1:1 response
    CURVE_S_CURVE,     ///< S-curve (gentle center, steep edges)
    CURVE_EXPONENTIAL, ///< Exponential curve (gentle start, steep end)
    CURVE_CUSTOM       ///< User-defined custom curve
};

/**
 * @brief Predefined filter levels for common use cases
 */
enum AxisFilterLevel {
    AXIS_FILTER_OFF,    ///< No filtering (raw values pass through)
    AXIS_FILTER_LOW,    ///< Light filtering for high-precision controls
    AXIS_FILTER_MEDIUM, ///< Moderate filtering for general use
    AXIS_FILTER_HIGH    ///< Heavy filtering for noisy or low-quality sensors
};

// =============================================================================
// AXIS FILTER CLASS
// =============================================================================

/**
 * @brief Advanced noise filtering and smoothing for analog axis values
 * 
 * This class provides multi-stage filtering:
 * 1. Noise threshold filtering - ignores small changes below threshold
 * 2. Velocity calculation - measures rate of change
 * 3. Adaptive smoothing - reduces smoothing during fast movements
 * 4. Emergency pass-through - bypasses smoothing for very fast movements
 * 
 * The filter adapts its behavior based on movement velocity to provide
 * both stability during slow movements and responsiveness during fast movements.
 */
class AxisFilter {
private:
    int32_t filteredValue = 0;        ///< Current filtered output value
    int32_t lastRawValue = 0;         ///< Previous raw input value
    int32_t lastProcessedValue = 0;   ///< Previous processed value for velocity calculation
    uint32_t lastUpdateTime = 0;      ///< Timestamp of last update (milliseconds)
    bool initialized = false;         ///< Whether filter has been initialized
    
    // Filter parameters
    int32_t noiseThreshold = 2;       ///< Minimum change required to update (0-10 typical)
    uint8_t smoothingFactor = 3;      ///< Exponential smoothing factor (0-7)
    int32_t velocityThreshold = 20;   ///< Speed threshold for adaptive smoothing
    AxisFilterLevel filterLevel = AXIS_FILTER_MEDIUM; ///< Current filter level

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
     * @brief Set predefined filter level
     * @param level Filter level (OFF, LOW, MEDIUM, HIGH)
     */
    void setLevel(AxisFilterLevel level);
    
    /**
     * @brief Set noise threshold
     * @param threshold Minimum change required to update value (0-10 recommended)
     * 
     * Lower values = more sensitive to small changes
     * Higher values = more stable, ignores small movements
     */
    void setNoiseThreshold(int32_t threshold);
    
    /**
     * @brief Set smoothing factor
     * @param factor Exponential smoothing factor (0-7)
     * 
     * 0 = no smoothing (immediate response)
     * 7 = maximum smoothing (very slow response)
     * Higher values = smoother but less responsive
     */
    void setSmoothingFactor(uint8_t factor);
    
    /**
     * @brief Set velocity threshold for adaptive smoothing
     * @param threshold Speed threshold for reducing smoothing (0-50 typical)
     * 
     * When movement velocity exceeds this threshold, smoothing is reduced
     * for better responsiveness during fast movements
     */
    void setVelocityThreshold(int32_t threshold);
    
    // Getters for current settings
    int32_t getNoiseThreshold() const { return noiseThreshold; }
    uint8_t getSmoothingFactor() const { return smoothingFactor; }
    int32_t getVelocityThreshold() const { return velocityThreshold; }
    AxisFilterLevel getFilterLevel() const { return filterLevel; }
};

// =============================================================================
// AXIS CURVE CLASS
// =============================================================================

/**
 * @brief Response curve shaping for analog axis values
 * 
 * This class applies response curves to modify the relationship between
 * input and output values. Useful for:
 * - Creating dead zones
 * - Adjusting sensitivity curves
 * - Implementing custom response characteristics
 * 
 * Supports linear interpolation between curve points for smooth transitions.
 */
class AxisCurve {
private:
    ResponseCurveType type = CURVE_LINEAR; ///< Current curve type
    int32_t customTable[11] = {0, 102, 204, 306, 408, 512, 614, 716, 818, 920, 1023}; ///< Custom curve points
    uint8_t points = 11; ///< Number of points in custom curve

public:
    /**
     * @brief Apply response curve to input value
     * @param input Input value (typically 0-1023 range)
     * @return Curve-shaped output value
     */
    int32_t apply(int32_t input);
    
    /**
     * @brief Set response curve type
     * @param newType Curve type (LINEAR, S_CURVE, EXPONENTIAL, CUSTOM)
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
// PRESET CURVE TABLES
// =============================================================================

/**
 * @brief Predefined response curve lookup tables
 * 
 * These tables define the shape of each curve type:
 * - Linear: Straight 1:1 mapping
 * - S-Curve: Gentle in center, steep at edges (good for flight controls)
 * - Exponential: Gentle at start, steep at end (good for throttles)
 */
static constexpr int32_t PRESET_CURVES[3][11] = {
    // CURVE_LINEAR: 1:1 linear response
    {0, 102, 204, 306, 408, 512, 614, 716, 818, 920, 1023},
    
    // CURVE_S_CURVE: Gentle center, steep edges
    {0, 10, 40, 120, 260, 512, 764, 904, 984, 1013, 1023},
    
    // CURVE_EXPONENTIAL: Gentle start, steep end
    {0, 5, 20, 45, 80, 125, 180, 245, 320, 405, 1023}
};

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Get preset curve table for a given curve type
 * @param type Curve type
 * @return Pointer to curve table (11 points)
 */
inline const int32_t* getPresetCurve(ResponseCurveType type) {
    switch (type) {
        case CURVE_LINEAR:      return PRESET_CURVES[0];
        case CURVE_S_CURVE:     return PRESET_CURVES[1];
        case CURVE_EXPONENTIAL: return PRESET_CURVES[2];
        default:                return PRESET_CURVES[0]; // Default to linear
    }
}

/**
 * @brief Get human-readable name for filter level
 * @param level Filter level
 * @return String representation
 */
inline const char* getFilterLevelName(AxisFilterLevel level) {
    switch (level) {
        case AXIS_FILTER_OFF:    return "Off";
        case AXIS_FILTER_LOW:    return "Low";
        case AXIS_FILTER_MEDIUM: return "Medium";
        case AXIS_FILTER_HIGH:   return "High";
        default:                 return "Unknown";
    }
}

/**
 * @brief Get human-readable name for curve type
 * @param type Curve type
 * @return String representation
 */
inline const char* getCurveTypeName(ResponseCurveType type) {
    switch (type) {
        case CURVE_LINEAR:      return "Linear";
        case CURVE_S_CURVE:     return "S-Curve";
        case CURVE_EXPONENTIAL: return "Exponential";
        case CURVE_CUSTOM:      return "Custom";
        default:                return "Unknown";
    }
}

#endif // AXIS_PROCESSING_H 