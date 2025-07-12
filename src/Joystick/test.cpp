#include <Arduino.h>
#include "Joystick.h"
#include "ConfigAxis.h"

// Create a MINIMAL joystick - only basic axes, completely unified system
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK, 
                   1, 0,     // Only 1 button, no hat switches
                   true, false, false,  // Only X axis enabled by default
                   false, false, false, // No Rx,Ry,Rz
                   false, false); // No rudder, throttle

void setup() {
    // Configure all user-defined axes
    setupUserAxes(Joystick);
    
    // Initialize joystick
    Joystick.begin();
    delay(1000);
}

void loop() {
    // Read all configured axes from user.h
    readUserAxes(Joystick);
    
 
}