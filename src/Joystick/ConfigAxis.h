#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "AnalogAxis.h"
#include "Joystick.h" // Add this line to ensure Joystick_ is defined

/*
 * HOTAS User Configuration
 * 
 * Configure your axes here by uncommenting and modifying the settings below.
 * Add more axes as needed by copying the pattern.
 */

// =============================================================================
// AXIS CONFIGURATION
// =============================================================================

// X-Axis (Main stick pitch)
#define USE_AXIS_X
#ifdef USE_AXIS_X
    #define AXIS_X_PIN              A0
    #define AXIS_X_MIN              0
    #define AXIS_X_MAX              1023
    #define AXIS_X_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_X_NOISE_THRESHOLD  3
    #define AXIS_X_SMOOTHING        2
    #define AXIS_X_VELOCITY         15
    #define AXIS_X_CURVE            CURVE_LINEAR
#endif

// Y-Axis (Main stick roll) - uncomment to enable
// #define USE_AXIS_Y
#ifdef USE_AXIS_Y
    #define AXIS_Y_PIN              A1
    #define AXIS_Y_MIN              0
    #define AXIS_Y_MAX              1023
    #define AXIS_Y_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_Y_NOISE_THRESHOLD  3
    #define AXIS_Y_SMOOTHING        2
    #define AXIS_Y_VELOCITY         15
    #define AXIS_Y_CURVE            CURVE_LINEAR
#endif

// Z-Axis - uncomment to enable
// #define USE_AXIS_Z
#ifdef USE_AXIS_Z
    #define AXIS_Z_PIN              A4
    #define AXIS_Z_MIN              0
    #define AXIS_Z_MAX              1023
    #define AXIS_Z_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_Z_NOISE_THRESHOLD  3
    #define AXIS_Z_SMOOTHING        2
    #define AXIS_Z_VELOCITY         15
    #define AXIS_Z_CURVE            CURVE_LINEAR
#endif

// RX-Axis - uncomment to enable
// #define USE_AXIS_RX
#ifdef USE_AXIS_RX
    #define AXIS_RX_PIN             A5
    #define AXIS_RX_MIN             0
    #define AXIS_RX_MAX             1023
    #define AXIS_RX_FILTER_LEVEL    AXIS_FILTER_MEDIUM
    #define AXIS_RX_NOISE_THRESHOLD 3
    #define AXIS_RX_SMOOTHING       2
    #define AXIS_RX_VELOCITY        15
    #define AXIS_RX_CURVE           CURVE_LINEAR
#endif

// RY-Axis - uncomment to enable
// #define USE_AXIS_RY
#ifdef USE_AXIS_RY
    #define AXIS_RY_PIN             A6
    #define AXIS_RY_MIN             0
    #define AXIS_RY_MAX             1023
    #define AXIS_RY_FILTER_LEVEL    AXIS_FILTER_MEDIUM
    #define AXIS_RY_NOISE_THRESHOLD 3
    #define AXIS_RY_SMOOTHING       2
    #define AXIS_RY_VELOCITY        15
    #define AXIS_RY_CURVE           CURVE_LINEAR
#endif

// RZ-Axis (Rudder/twist) - uncomment to enable
// #define USE_AXIS_RZ
#ifdef USE_AXIS_RZ
    #define AXIS_RZ_PIN             A2
    #define AXIS_RZ_MIN             0
    #define AXIS_RZ_MAX             1023
    #define AXIS_RZ_FILTER_LEVEL    AXIS_FILTER_HIGH
    #define AXIS_RZ_NOISE_THRESHOLD 4
    #define AXIS_RZ_SMOOTHING       3
    #define AXIS_RZ_VELOCITY        25
    #define AXIS_RZ_CURVE           CURVE_LINEAR
#endif

// S1-Axis (Throttle) - uncomment to enable
// #define USE_AXIS_S1
#ifdef USE_AXIS_S1
    #define AXIS_S1_PIN             A3
    #define AXIS_S1_MIN             0
    #define AXIS_S1_MAX             1023
    #define AXIS_S1_FILTER_LEVEL    AXIS_FILTER_LOW
    #define AXIS_S1_NOISE_THRESHOLD 2
    #define AXIS_S1_SMOOTHING       1
    #define AXIS_S1_VELOCITY        10
    #define AXIS_S1_CURVE           CURVE_LINEAR
#endif

// S2-Axis (Second throttle/slider) - uncomment to enable
// #define USE_AXIS_S2
#ifdef USE_AXIS_S2
    #define AXIS_S2_PIN             A7
    #define AXIS_S2_MIN             0
    #define AXIS_S2_MAX             1023
    #define AXIS_S2_FILTER_LEVEL    AXIS_FILTER_LOW
    #define AXIS_S2_NOISE_THRESHOLD 2
    #define AXIS_S2_SMOOTHING       1
    #define AXIS_S2_VELOCITY        10
    #define AXIS_S2_CURVE           CURVE_LINEAR
#endif

// =============================================================================
// SETUP FUNCTION - DO NOT MODIFY
// =============================================================================

inline void setupUserAxes(Joystick_& joystick) {
    #ifdef USE_AXIS_X
        joystick.setAxisPin(AnalogAxisManager::AXIS_X, AXIS_X_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_X, AXIS_X_MIN, AXIS_X_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_X, AXIS_X_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_X, AXIS_X_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_X, AXIS_X_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_X, AXIS_X_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_X, AXIS_X_CURVE);
    #endif
    
    #ifdef USE_AXIS_Y
        joystick.setAxisPin(AnalogAxisManager::AXIS_Y, AXIS_Y_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_Y, AXIS_Y_MIN, AXIS_Y_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_Y, AXIS_Y_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_Y, AXIS_Y_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_Y, AXIS_Y_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_Y, AXIS_Y_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_Y, AXIS_Y_CURVE);
    #endif
    
    #ifdef USE_AXIS_Z
        joystick.setAxisPin(AnalogAxisManager::AXIS_Z, AXIS_Z_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_Z, AXIS_Z_MIN, AXIS_Z_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_Z, AXIS_Z_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_Z, AXIS_Z_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_Z, AXIS_Z_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_Z, AXIS_Z_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_Z, AXIS_Z_CURVE);
    #endif
    
    #ifdef USE_AXIS_RX
        joystick.setAxisPin(AnalogAxisManager::AXIS_RX, AXIS_RX_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_RX, AXIS_RX_MIN, AXIS_RX_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_RX, AXIS_RX_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_RX, AXIS_RX_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_RX, AXIS_RX_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_RX, AXIS_RX_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_RX, AXIS_RX_CURVE);
    #endif
    
    #ifdef USE_AXIS_RY
        joystick.setAxisPin(AnalogAxisManager::AXIS_RY, AXIS_RY_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_RY, AXIS_RY_MIN, AXIS_RY_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_RY, AXIS_RY_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_RY, AXIS_RY_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_RY, AXIS_RY_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_RY, AXIS_RY_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_RY, AXIS_RY_CURVE);
    #endif
    
    #ifdef USE_AXIS_RZ
        joystick.setAxisPin(AnalogAxisManager::AXIS_RZ, AXIS_RZ_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_RZ, AXIS_RZ_MIN, AXIS_RZ_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_RZ, AXIS_RZ_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_RZ, AXIS_RZ_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_RZ, AXIS_RZ_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_RZ, AXIS_RZ_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_RZ, AXIS_RZ_CURVE);
    #endif
    
    #ifdef USE_AXIS_S1
        joystick.setAxisPin(AnalogAxisManager::AXIS_S1, AXIS_S1_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_S1, AXIS_S1_MIN, AXIS_S1_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_S1, AXIS_S1_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_S1, AXIS_S1_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_S1, AXIS_S1_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_S1, AXIS_S1_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_S1, AXIS_S1_CURVE);
    #endif
    
    #ifdef USE_AXIS_S2
        joystick.setAxisPin(AnalogAxisManager::AXIS_S2, AXIS_S2_PIN);
        joystick.setAxisRange(AnalogAxisManager::AXIS_S2, AXIS_S2_MIN, AXIS_S2_MAX);
        joystick.setAxisFilterLevel(AnalogAxisManager::AXIS_S2, AXIS_S2_FILTER_LEVEL);
        joystick.setAxisNoiseThreshold(AnalogAxisManager::AXIS_S2, AXIS_S2_NOISE_THRESHOLD);
        joystick.setAxisSmoothingFactor(AnalogAxisManager::AXIS_S2, AXIS_S2_SMOOTHING);
        joystick.setAxisVelocityThreshold(AnalogAxisManager::AXIS_S2, AXIS_S2_VELOCITY);
        joystick.setAxisResponseCurve(AnalogAxisManager::AXIS_S2, AXIS_S2_CURVE);
    #endif
}

inline void readUserAxes(Joystick_& joystick) {
    #ifdef USE_AXIS_X
        int xVal = analogRead(AXIS_X_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_X, xVal);
    #endif
    
    #ifdef USE_AXIS_Y
        int yVal = analogRead(AXIS_Y_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_Y, yVal);
    #endif
    
    #ifdef USE_AXIS_Z
        int zVal = analogRead(AXIS_Z_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_Z, zVal);
    #endif
    
    #ifdef USE_AXIS_RX
        int rxVal = analogRead(AXIS_RX_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_RX, rxVal);
    #endif
    
    #ifdef USE_AXIS_RY
        int ryVal = analogRead(AXIS_RY_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_RY, ryVal);
    #endif
    
    #ifdef USE_AXIS_RZ
        int rzVal = analogRead(AXIS_RZ_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_RZ, rzVal);
    #endif
    
    #ifdef USE_AXIS_S1
        int s1Val = analogRead(AXIS_S1_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_S1, s1Val);
    #endif
    
    #ifdef USE_AXIS_S2
        int s2Val = analogRead(AXIS_S2_PIN);
        joystick.setAxis(AnalogAxisManager::AXIS_S2, s2Val);
    #endif
}

#endif // USER_CONFIG_H
