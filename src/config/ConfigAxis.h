#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "../inputs/analog/AnalogAxis.h"
#include "../inputs/analog/AxisProcessing.h"
#include "../rp2040/JoystickWrapper.h"

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
    #define AXIS_X_CURVE            CURVE_CUSTOM
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
    #define AXIS_Y_CURVE            CURVE_CUSTOM
#endif

// Z-Axis - uncomment to enable
// #define USE_AXIS_Z
#ifdef USE_AXIS_Z
    #define AXIS_Z_PIN              A4
    #define AXIS_Z_MIN              0
    #define AXIS_Z_MAX              32767
    #define AXIS_Z_FILTER_LEVEL     AXIS_FILTER_EWMA
    #define AXIS_Z_EWMA_ALPHA       30
    #define AXIS_Z_DEADBAND         0
    #define AXIS_Z_CURVE            CURVE_CUSTOM
#endif

// RX-Axis - uncomment to enable
// #define USE_AXIS_RX
#ifdef USE_AXIS_RX
    #define AXIS_RX_PIN             A5
    #define AXIS_RX_MIN             0
    #define AXIS_RX_MAX             32767
    #define AXIS_RX_FILTER_LEVEL    AXIS_FILTER_EWMA
    #define AXIS_RX_EWMA_ALPHA      30
    #define AXIS_RX_DEADBAND        0
    #define AXIS_RX_CURVE           CURVE_CUSTOM
#endif

// RY-Axis - uncomment to enable
// #define USE_AXIS_RY
#ifdef USE_AXIS_RY
    #define AXIS_RY_PIN             A6
    #define AXIS_RY_MIN             0
    #define AXIS_RY_MAX             32767
    #define AXIS_RY_FILTER_LEVEL    AXIS_FILTER_EWMA
    #define AXIS_RY_EWMA_ALPHA      30
    #define AXIS_RY_DEADBAND        0
    #define AXIS_RY_CURVE           CURVE_CUSTOM
#endif

// RZ-Axis (Rudder/twist) - uncomment to enable
// #define USE_AXIS_RZ
#ifdef USE_AXIS_RZ
    #define AXIS_RZ_PIN             A2
    #define AXIS_RZ_MIN             0
    #define AXIS_RZ_MAX             32767
    #define AXIS_RZ_FILTER_LEVEL    AXIS_FILTER_EWMA
    #define AXIS_RZ_EWMA_ALPHA      30
    #define AXIS_RZ_DEADBAND        0
    #define AXIS_RZ_CURVE           CURVE_CUSTOM
#endif

// S1-Axis (Throttle) - uncomment to enable
// #define USE_AXIS_S1
#ifdef USE_AXIS_S1
    #define AXIS_S1_PIN             A3
    #define AXIS_S1_MIN             0
    #define AXIS_S1_MAX             32767
    #define AXIS_S1_FILTER_LEVEL    AXIS_FILTER_EWMA
    #define AXIS_S1_EWMA_ALPHA      30
    #define AXIS_S1_DEADBAND        0
    #define AXIS_S1_CURVE           CURVE_CUSTOM
#endif

// S2-Axis (Second throttle/slider) - uncomment to enable
// #define USE_AXIS_S2
#ifdef USE_AXIS_S2
    #define AXIS_S2_PIN             A7
    #define AXIS_S2_MIN             0
    #define AXIS_S2_MAX             1023
    #define AXIS_S2_FILTER_LEVEL    AXIS_FILTER_EWMA
    #define AXIS_S2_EWMA_ALPHA      30
    #define AXIS_S2_DEADBAND        0
    #define AXIS_S2_CURVE           CURVE_CUSTOM
#endif

// =============================================================================
// DYNAMIC AXIS MAPPING FUNCTION (managed below)
// =============================================================================

// Unified axis descriptor consolidating setup & publish data
struct AxisDescriptor {
    uint8_t idx;
    int pin;
    int minv;
    int maxv;
    AxisFilterLevel filter;
    uint32_t alpha;
    int deadband;
    ResponseCurveType curve;
};

static const AxisDescriptor axisDescriptors[] = {
#ifdef USE_AXIS_X
    { AnalogAxisManager::AXIS_X, AXIS_X_PIN, AXIS_X_MIN, AXIS_X_MAX, AXIS_X_FILTER_LEVEL, AXIS_X_EWMA_ALPHA, AXIS_X_DEADBAND, AXIS_X_CURVE },
#endif
#ifdef USE_AXIS_Y
    { AnalogAxisManager::AXIS_Y, AXIS_Y_PIN, AXIS_Y_MIN, AXIS_Y_MAX, AXIS_Y_FILTER_LEVEL, AXIS_Y_EWMA_ALPHA, AXIS_Y_DEADBAND, AXIS_Y_CURVE },
#endif
#ifdef USE_AXIS_Z
    { AnalogAxisManager::AXIS_Z, AXIS_Z_PIN, AXIS_Z_MIN, AXIS_Z_MAX, AXIS_Z_FILTER_LEVEL, AXIS_Z_EWMA_ALPHA, AXIS_Z_DEADBAND, AXIS_Z_CURVE },
#endif
#ifdef USE_AXIS_RX
    { AnalogAxisManager::AXIS_RX, AXIS_RX_PIN, AXIS_RX_MIN, AXIS_RX_MAX, AXIS_RX_FILTER_LEVEL, AXIS_RX_EWMA_ALPHA, AXIS_RX_DEADBAND, AXIS_RX_CURVE },
#endif
#ifdef USE_AXIS_RY
    { AnalogAxisManager::AXIS_RY, AXIS_RY_PIN, AXIS_RY_MIN, AXIS_RY_MAX, AXIS_RY_FILTER_LEVEL, AXIS_RY_EWMA_ALPHA, AXIS_RY_DEADBAND, AXIS_RY_CURVE },
#endif
#ifdef USE_AXIS_RZ
    { AnalogAxisManager::AXIS_RZ, AXIS_RZ_PIN, AXIS_RZ_MIN, AXIS_RZ_MAX, AXIS_RZ_FILTER_LEVEL, AXIS_RZ_EWMA_ALPHA, AXIS_RZ_DEADBAND, AXIS_RZ_CURVE },
#endif
#ifdef USE_AXIS_S1
    { AnalogAxisManager::AXIS_S1, AXIS_S1_PIN, AXIS_S1_MIN, AXIS_S1_MAX, AXIS_S1_FILTER_LEVEL, AXIS_S1_EWMA_ALPHA, AXIS_S1_DEADBAND, AXIS_S1_CURVE },
#endif
#ifdef USE_AXIS_S2
    { AnalogAxisManager::AXIS_S2, AXIS_S2_PIN, AXIS_S2_MIN, AXIS_S2_MAX, AXIS_S2_FILTER_LEVEL, AXIS_S2_EWMA_ALPHA, AXIS_S2_DEADBAND, AXIS_S2_CURVE },
#endif
};

inline bool isAdsPin(int p){ return p >= 100 && p <= 103; }

// =============================================================================
 // SETUP FUNCTION - DO NOT MODIFY
 // Initializes ADS1115 only if any axis uses ADS1115_CH*.
 // Axis parameters are applied in readUserAxes() during first run.
 // =============================================================================

// Axis setup now uses data-driven descriptors (see axisDescriptors above); no macros required.

inline void setupUserAxes(Joystick_& joystick) {
    (void)joystick; // currently unused
    bool needsADS1115 = false;
    for (auto &d : axisDescriptors) {
        if (isAdsPin(d.pin)) { needsADS1115 = true; break; }
    }
    if (needsADS1115) initializeADS1115IfNeeded();
}

inline void readUserAxes(Joystick_& joystick) {
    static AnalogAxisManager axisManager;
    static bool configured = false;
    if (!configured) {
        for (auto &d : axisDescriptors) {
            axisManager.setAxisPin(d.idx, d.pin);
            axisManager.setAxisRange(d.idx, d.minv, d.maxv);
            axisManager.setAxisFilterLevel(d.idx, d.filter);
            axisManager.setAxisEwmaAlpha(d.idx, d.alpha);
            axisManager.setAxisDeadbandSize(d.idx, d.deadband);
            axisManager.setAxisResponseCurve(d.idx, d.curve);
            axisManager.enableAxis(d.idx, true);
        }
        configured = true;
    }
    axisManager.readAllAxes();
    for (auto &d : axisDescriptors) {
        joystick.setAxis(d.idx, axisManager.getAxisValue(d.idx));
    }
}

#endif // USER_CONFIG_H
