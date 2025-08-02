#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "AnalogAxis.h"
#include "AxisProcessing.h"
#include "JoystickWrapper.h"

/*
 * HOTAS Axis Configuration (runtime behavior reflects the whole codebase)
 *
 * Pipeline per axis (AnalogAxisManager):
 *   Raw hardware -> map to user range -> Deadband (dynamic around current pos)
 *   -> Filter (adaptive smoothing or EWMA) -> Response Curve -> HID mapping
 *
 * Hardware ranges:
 *   - Built-in analog pins: 10-bit (0..1023)
 *   - ADS1115 channels (ADS1115_CH0..CH3): cached 16-bit (0..16383), read in round-robin
 *
 * HID mapping:
 *   - Processed user range (e.g., 0..32767) is mapped to -32767..32767 for rp2040-HID.
 *
 * FILTER_LEVEL options (AxisProcessing.cpp):
 *   AXIS_FILTER_OFF    - Pass-through (no smoothing)
 *   AXIS_FILTER_LOW    - Light smoothing, low velocity threshold
 *   AXIS_FILTER_MEDIUM - Moderate smoothing (default behavior)
 *   AXIS_FILTER_HIGH   - Heavy smoothing for noisy inputs
 *   AXIS_FILTER_EWMA   - EWMA filter; uses AXIS_*_EWMA_ALPHA (0..1000), higher alpha = more responsive
 *
 * Deadband:
 *   - Dynamic around current position; activates when average movement is low to hold value steady.
 *   - Applied BEFORE filtering/curves; good for eliminating jitter at rest.
 *
 * ADS1115 behavior:
 *   - Automatically initialized if any axis pin is ADS1115_CH0..CH3.
 *   - Channels registered once and read in a non-blocking round-robin (20 ms per channel),
 *     with latest values cached to avoid blocking and prevent encoder lag.
 *
 * Enabling axes:
 *   - Uncomment USE_AXIS_* and set AXIS_*_* values. AxisManager is configured once on first read.
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
 // Initializes ADS1115 only if any axis uses ADS1115_CH*.
 // Axis parameters are applied in readUserAxes() during first run.
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
    // Configure once, then read/process all enabled axes each loop.
    // AnalogAxisManager enforces ~5 ms read cadence and uses cached ADS1115 values (round-robin).
    static AnalogAxisManager axisManager;
    static bool configured = false;
    
    // Configure axis manager on first run (per-axis pin, range, filter, EWMA alpha, deadband, curve)
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
