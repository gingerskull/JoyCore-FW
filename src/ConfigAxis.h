#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "AnalogAxis.h"
#include "JoystickWrapper.h"

/*
 * HOTAS User Configuration
 * 
 * Configure your axes here by uncommenting and modifying the settings below.
 * Add more axes as needed by copying the pattern.
 * 
 * FILTER_LEVEL options:
 *   AXIS_FILTER_OFF    - No filtering (raw values)
 *   AXIS_FILTER_LOW    - Light filtering (smoothing=1, noise=1, velocity=15)
 *   AXIS_FILTER_MEDIUM - Moderate filtering (smoothing=3, noise=2, velocity=20) 
 *   AXIS_FILTER_HIGH   - Heavy filtering (smoothing=4, noise=3, velocity=25)
 * 
 * NOISE_THRESHOLD: Minimum change required to update value (0-10 recommended)
 *   Lower values = more sensitive, higher values = more stable
 * 
 * SMOOTHING: Exponential smoothing factor (0-7)
 *   0 = no smoothing, 7 = maximum smoothing
 *   Higher values = smoother but less responsive
 * 
 * VELOCITY: Speed threshold for adaptive smoothing (0-50)
 *   Higher values = less smoothing during fast movements
 * 
 * CURVE options:
 *   CURVE_LINEAR      - Linear response (1:1 mapping)
 *   CURVE_S_CURVE     - S-curve (gentle at center, steeper at edges)
 *   CURVE_EXPONENTIAL - Exponential (gentle at start, steep at end)
 *   CURVE_CUSTOM      - Custom curve (define your own)
 * 
 * AXIS PIN CONFIGURATION: ANALOG PINS AND ADS1115 CHANNELS
 * 
 * To use a built-in analog pin for an axis, set AXIS_X_PIN, AXIS_Y_PIN, etc. to A0, A1, etc.
 * To use an external ADS1115 ADC channel, set the pin to one of:
 *   ADS1115_CH0, ADS1115_CH1, ADS1115_CH2, ADS1115_CH3
 * 
 * Example:
 *   #define AXIS_X_PIN ADS1115_CH0  // Uses ADS1115 channel 0 (high resolution)
 *   #define AXIS_Y_PIN A1           // Uses built-in analog pin A1
 *
 * The ADS1115 is automatically initialized when any axis uses ADS1115 channels.
 * No manual initialization required - just change the pin definition!
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
    // Check if any axis uses ADS1115 channels and initialize if needed
    bool needsADS1115 = false;
    #ifdef USE_AXIS_X
        if (AXIS_X_PIN >= 100 && AXIS_X_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_Y
        if (AXIS_Y_PIN >= 100 && AXIS_Y_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_Z
        if (AXIS_Z_PIN >= 100 && AXIS_Z_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_RX
        if (AXIS_RX_PIN >= 100 && AXIS_RX_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_RY
        if (AXIS_RY_PIN >= 100 && AXIS_RY_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_RZ
        if (AXIS_RZ_PIN >= 100 && AXIS_RZ_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_S1
        if (AXIS_S1_PIN >= 100 && AXIS_S1_PIN <= 103) needsADS1115 = true;
    #endif
    #ifdef USE_AXIS_S2
        if (AXIS_S2_PIN >= 100 && AXIS_S2_PIN <= 103) needsADS1115 = true;
    #endif
    
    // Initialize ADS1115 if any axis needs it
    if (needsADS1115) {
        initializeADS1115IfNeeded();
    }

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
    // Use the AnalogAxisManager to read all axes (handles both analog pins and ADS1115)
    static AnalogAxisManager axisManager;
    
    #ifdef USE_AXIS_X
        axisManager.setAxisPin(AnalogAxisManager::AXIS_X, AXIS_X_PIN);
        int32_t xVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_X);
        joystick.setAxis(AnalogAxisManager::AXIS_X, xVal);
    #endif
    
    #ifdef USE_AXIS_Y
        axisManager.setAxisPin(AnalogAxisManager::AXIS_Y, AXIS_Y_PIN);
        int32_t yVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_Y);
        joystick.setAxis(AnalogAxisManager::AXIS_Y, yVal);
    #endif
    
    #ifdef USE_AXIS_Z
        axisManager.setAxisPin(AnalogAxisManager::AXIS_Z, AXIS_Z_PIN);
        int32_t zVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_Z);
        joystick.setAxis(AnalogAxisManager::AXIS_Z, zVal);
    #endif
    
    #ifdef USE_AXIS_RX
        axisManager.setAxisPin(AnalogAxisManager::AXIS_RX, AXIS_RX_PIN);
        int32_t rxVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_RX);
        joystick.setAxis(AnalogAxisManager::AXIS_RX, rxVal);
    #endif
    
    #ifdef USE_AXIS_RY
        axisManager.setAxisPin(AnalogAxisManager::AXIS_RY, AXIS_RY_PIN);
        int32_t ryVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_RY);
        joystick.setAxis(AnalogAxisManager::AXIS_RY, ryVal);
    #endif
    
    #ifdef USE_AXIS_RZ
        axisManager.setAxisPin(AnalogAxisManager::AXIS_RZ, AXIS_RZ_PIN);
        int32_t rzVal = axisManager.readAxisRaw(AnalogAxisManager::AXIS_RZ);
        joystick.setAxis(AnalogAxisManager::AXIS_RZ, rzVal);
    #endif
    
    #ifdef USE_AXIS_S1
        axisManager.setAxisPin(AnalogAxisManager::AXIS_S1, AXIS_S1_PIN);
        int32_t s1Val = axisManager.readAxisRaw(AnalogAxisManager::AXIS_S1);
        joystick.setAxis(AnalogAxisManager::AXIS_S1, s1Val);
    #endif
    
    #ifdef USE_AXIS_S2
        axisManager.setAxisPin(AnalogAxisManager::AXIS_S2, AXIS_S2_PIN);
        int32_t s2Val = axisManager.readAxisRaw(AnalogAxisManager::AXIS_S2);
        joystick.setAxis(AnalogAxisManager::AXIS_S2, s2Val);
    #endif
}

#endif // USER_CONFIG_H
