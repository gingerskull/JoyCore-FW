#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "AnalogAxis.h"
#include "AxisProcessing.h"
#include "JoystickWrapper.h"

/*
 * HOTAS User Configuration
 * 
 * Configure your axes here by uncommenting and modifying the settings below.
 * Add more axes as needed by copying the pattern.
 * 
 * FILTER_LEVEL options:
 *   AXIS_FILTER_OFF    - No filtering (raw values)
 *   AXIS_FILTER_LOW    - Light filtering
 *   AXIS_FILTER_MEDIUM - Moderate filtering
 *   AXIS_FILTER_HIGH   - Heavy filtering
 *   AXIS_FILTER_EWMA   - EWMA filtering (configurable alpha parameter)
 * 
 * EWMA_ALPHA: Alpha parameter for EWMA filtering (0-1000, scaled by 1000)
 *   Higher values = less smoothing (more responsive)
 *   Lower values = more smoothing (less responsive)
 *   Common values: 30 (0.03), 100 (0.1), 200 (0.2), 500 (0.5)
 *   Only used when FILTER_LEVEL is set to AXIS_FILTER_EWMA
 * 
 * DEADBAND: Dead zone around current axis position (0 = disabled)
 *   Prevents small fluctuations when user stops moving the control
 *   Typical values: 0 (off), 500-1000 (light), 1000-2000 (medium), 2000-5000 (heavy)
 *   Works around the CURRENT axis value, not a fixed center point
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
    #define AXIS_X_PIN              A1
    #define AXIS_X_MIN              0
    #define AXIS_X_MAX              32767
    #define AXIS_X_FILTER_LEVEL     AXIS_FILTER_EWMA
    #define AXIS_X_EWMA_ALPHA       200
    #define AXIS_X_DEADBAND         250
    #define AXIS_X_CURVE            CURVE_LINEAR
#endif

//Y-Axis (Main stick yaw)
#define USE_AXIS_Y
#ifdef USE_AXIS_Y
    #define AXIS_Y_PIN              A2
    #define AXIS_Y_MIN              0
    #define AXIS_Y_MAX              32767
    #define AXIS_Y_FILTER_LEVEL     AXIS_FILTER_EWMA
    #define AXIS_Y_EWMA_ALPHA       200
    #define AXIS_Y_DEADBAND         250
    #define AXIS_Y_CURVE            CURVE_LINEAR
#endif

// Z-Axis - uncomment to enable
// #define USE_AXIS_Z
#ifdef USE_AXIS_Z
    #define AXIS_Z_PIN              A4
    #define AXIS_Z_MIN              0
    #define AXIS_Z_MAX              32767
    #define AXIS_Z_FILTER_LEVEL     AXIS_FILTER_MEDIUM
    #define AXIS_Z_EWMA_ALPHA       30
    #define AXIS_Z_DEADBAND         0
    #define AXIS_Z_CURVE            CURVE_LINEAR
#endif

// RX-Axis - uncomment to enable
// #define USE_AXIS_RX
#ifdef USE_AXIS_RX
    #define AXIS_RX_PIN             A5
    #define AXIS_RX_MIN             0
    #define AXIS_RX_MAX             32767
    #define AXIS_RX_FILTER_LEVEL    AXIS_FILTER_MEDIUM
    #define AXIS_RX_EWMA_ALPHA      30
    #define AXIS_RX_DEADBAND        0
    #define AXIS_RX_CURVE           CURVE_LINEAR
#endif

// RY-Axis - uncomment to enable
// #define USE_AXIS_RY
#ifdef USE_AXIS_RY
    #define AXIS_RY_PIN             A6
    #define AXIS_RY_MIN             0
    #define AXIS_RY_MAX             32767
    #define AXIS_RY_FILTER_LEVEL    AXIS_FILTER_MEDIUM
    #define AXIS_RY_EWMA_ALPHA      30
    #define AXIS_RY_DEADBAND        0
    #define AXIS_RY_CURVE           CURVE_LINEAR
#endif

// RZ-Axis (Rudder/twist) - uncomment to enable
// #define USE_AXIS_RZ
#ifdef USE_AXIS_RZ
    #define AXIS_RZ_PIN             A2
    #define AXIS_RZ_MIN             0
    #define AXIS_RZ_MAX             32767
    #define AXIS_RZ_FILTER_LEVEL    AXIS_FILTER_HIGH
    #define AXIS_RZ_EWMA_ALPHA      30
    #define AXIS_RZ_DEADBAND        0
    #define AXIS_RZ_CURVE           CURVE_LINEAR
#endif

// S1-Axis (Throttle) - uncomment to enable
// #define USE_AXIS_S1
#ifdef USE_AXIS_S1
    #define AXIS_S1_PIN             A3
    #define AXIS_S1_MIN             0
    #define AXIS_S1_MAX             32767
    #define AXIS_S1_FILTER_LEVEL    AXIS_FILTER_LOW
    #define AXIS_S1_EWMA_ALPHA      30
    #define AXIS_S1_DEADBAND        0
    #define AXIS_S1_CURVE           CURVE_LINEAR
#endif

// S2-Axis (Second throttle/slider) - uncomment to enable
// #define USE_AXIS_S2
#ifdef USE_AXIS_S2
    #define AXIS_S2_PIN             A7
    #define AXIS_S2_MIN             0
    #define AXIS_S2_MAX             1023
    #define AXIS_S2_FILTER_LEVEL    AXIS_FILTER_LOW
    #define AXIS_S2_EWMA_ALPHA      30
    #define AXIS_S2_DEADBAND        0
    #define AXIS_S2_CURVE           CURVE_LINEAR
#endif

// =============================================================================
// DYNAMIC AXIS MAPPING FUNCTION
// =============================================================================



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
    
    // Note: Axis configuration is now handled directly in readUserAxes()
    // using AnalogAxisManager instead of going through joystick wrapper
}

inline void readUserAxes(Joystick_& joystick) {
    // Use the AnalogAxisManager to read all axes (handles both analog pins and ADS1115)
    static AnalogAxisManager axisManager;
    static bool configured = false;
    
    // Configure axis manager on first run
    if (!configured) {
        #ifdef USE_AXIS_X
            axisManager.setAxisPin(AnalogAxisManager::AXIS_X, AXIS_X_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_X, AXIS_X_MIN, AXIS_X_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_X, AXIS_X_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_X, AXIS_X_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_X, AXIS_X_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_X, AXIS_X_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_X, true);
        #endif
        
        #ifdef USE_AXIS_Y
            axisManager.setAxisPin(AnalogAxisManager::AXIS_Y, AXIS_Y_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_Y, AXIS_Y_MIN, AXIS_Y_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_Y, AXIS_Y_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_Y, AXIS_Y_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_Y, AXIS_Y_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_Y, AXIS_Y_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_Y, true);
        #endif
        
        #ifdef USE_AXIS_Z
            axisManager.setAxisPin(AnalogAxisManager::AXIS_Z, AXIS_Z_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_Z, AXIS_Z_MIN, AXIS_Z_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_Z, AXIS_Z_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_Z, AXIS_Z_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_Z, AXIS_Z_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_Z, AXIS_Z_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_Z, true);
        #endif
        
        #ifdef USE_AXIS_RX
            axisManager.setAxisPin(AnalogAxisManager::AXIS_RX, AXIS_RX_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_RX, AXIS_RX_MIN, AXIS_RX_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_RX, AXIS_RX_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_RX, AXIS_RX_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_RX, AXIS_RX_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_RX, AXIS_RX_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_RX, true);
        #endif
        
        #ifdef USE_AXIS_RY
            axisManager.setAxisPin(AnalogAxisManager::AXIS_RY, AXIS_RY_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_RY, AXIS_RY_MIN, AXIS_RY_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_RY, AXIS_RY_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_RY, AXIS_RY_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_RY, AXIS_RY_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_RY, AXIS_RY_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_RY, true);
        #endif
        
        #ifdef USE_AXIS_RZ
            axisManager.setAxisPin(AnalogAxisManager::AXIS_RZ, AXIS_RZ_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_RZ, AXIS_RZ_MIN, AXIS_RZ_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_RZ, AXIS_RZ_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_RZ, AXIS_RZ_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_RZ, AXIS_RZ_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_RZ, AXIS_RZ_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_RZ, true);
        #endif
        
        #ifdef USE_AXIS_S1
            axisManager.setAxisPin(AnalogAxisManager::AXIS_S1, AXIS_S1_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_S1, AXIS_S1_MIN, AXIS_S1_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_S1, AXIS_S1_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_S1, AXIS_S1_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_S1, AXIS_S1_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_S1, AXIS_S1_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_S1, true);
        #endif
        
        #ifdef USE_AXIS_S2
            axisManager.setAxisPin(AnalogAxisManager::AXIS_S2, AXIS_S2_PIN);
            axisManager.setAxisRange(AnalogAxisManager::AXIS_S2, AXIS_S2_MIN, AXIS_S2_MAX);
            axisManager.setAxisFilterLevel(AnalogAxisManager::AXIS_S2, AXIS_S2_FILTER_LEVEL);
            axisManager.setAxisEwmaAlpha(AnalogAxisManager::AXIS_S2, AXIS_S2_EWMA_ALPHA);
            axisManager.setAxisDeadbandSize(AnalogAxisManager::AXIS_S2, AXIS_S2_DEADBAND);
            axisManager.setAxisResponseCurve(AnalogAxisManager::AXIS_S2, AXIS_S2_CURVE);
            axisManager.enableAxis(AnalogAxisManager::AXIS_S2, true);
        #endif
        
        configured = true;
    }
    
    // Read and process all enabled axes using the AnalogAxisManager
    axisManager.readAllAxes();
    
    // Set joystick values for all enabled axes
    #ifdef USE_AXIS_X
        int32_t processedXVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_X);
        joystick.setAxis(AnalogAxisManager::AXIS_X, processedXVal);
    #endif
    
    #ifdef USE_AXIS_Y
        int32_t processedYVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_Y);
        joystick.setAxis(AnalogAxisManager::AXIS_Y, processedYVal);
    #endif
    
    #ifdef USE_AXIS_Z
        int32_t processedZVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_Z);
        joystick.setAxis(AnalogAxisManager::AXIS_Z, processedZVal);
    #endif
    
    #ifdef USE_AXIS_RX
        int32_t processedRxVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_RX);
        joystick.setAxis(AnalogAxisManager::AXIS_RX, processedRxVal);
    #endif
    
    #ifdef USE_AXIS_RY
        int32_t processedRyVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_RY);
        joystick.setAxis(AnalogAxisManager::AXIS_RY, processedRyVal);
    #endif
    
    #ifdef USE_AXIS_RZ
        int32_t processedRzVal = axisManager.getAxisValue(AnalogAxisManager::AXIS_RZ);
        joystick.setAxis(AnalogAxisManager::AXIS_RZ, processedRzVal);
    #endif
    
    #ifdef USE_AXIS_S1
        int32_t processedS1Val = axisManager.getAxisValue(AnalogAxisManager::AXIS_S1);
        joystick.setAxis(AnalogAxisManager::AXIS_S1, processedS1Val);
    #endif
    
    #ifdef USE_AXIS_S2
        int32_t processedS2Val = axisManager.getAxisValue(AnalogAxisManager::AXIS_S2);
        joystick.setAxis(AnalogAxisManager::AXIS_S2, processedS2Val);
    #endif
}

#endif // USER_CONFIG_H
