// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"

// HID Report structure for high-capacity gamepad
// 128 buttons (16 bytes) + 16 axes (32 bytes) = 48 bytes total
// Hat switches temporarily removed due to phantom input issues
typedef struct __attribute__((packed)) {
    uint8_t buttons[16];      // 128 buttons, 1 bit each (16 bytes)
    int16_t axes[16];         // 16 axes, 16-bit signed values (32 bytes)
    // uint8_t hats;          // Hat switches temporarily disabled
    // uint8_t hats2;         // Hat switches temporarily disabled
} joycore_gamepad_report_t;

// Hat switch direction values (4-bit values)
#define HAT_DIR_N   0   // North
#define HAT_DIR_NE  1   // Northeast  
#define HAT_DIR_E   2   // East
#define HAT_DIR_SE  3   // Southeast
#define HAT_DIR_S   4   // South
#define HAT_DIR_SW  5   // Southwest
#define HAT_DIR_W   6   // West
#define HAT_DIR_NW  7   // Northwest
#define HAT_DIR_C   8   // Center/neutral

class TinyUSBGamepad {
private:
    Adafruit_USBD_HID _usb_hid;
    joycore_gamepad_report_t _report;
    joycore_gamepad_report_t _prev_report;
    bool _auto_send;
    uint32_t _last_send_time;
    static constexpr uint32_t MIN_SEND_INTERVAL_US = 1000; // 1ms = 1000Hz max
    
    // State change detection
    bool _state_changed;
    
    // Temporary disable hat switches due to phantom inputs
    bool _hat_switches_disabled;
    
    // Custom HID descriptor for 128 buttons, 16 axes, 4 hat switches
    static const uint8_t _hid_descriptor[];
    static const uint16_t _hid_descriptor_len;
    
public:
    TinyUSBGamepad();
    ~TinyUSBGamepad();
    
    // Initialization
    bool begin(bool auto_send = true);
    void end();
    
    // Button methods (0-127)
    void setButton(uint8_t button, bool pressed);
    void pressButton(uint8_t button);
    void releaseButton(uint8_t button);
    void releaseAllButtons();
    
    // Axis methods (0-15), values -32767 to 32767
    void setAxis(uint8_t axis, int16_t value);
    void setX(int16_t value)        { setAxis(0, value); }
    void setY(int16_t value)        { setAxis(1, value); }
    void setZ(int16_t value)        { setAxis(2, value); }
    void setRx(int16_t value)       { setAxis(3, value); }
    void setRy(int16_t value)       { setAxis(4, value); }
    void setRz(int16_t value)       { setAxis(5, value); }
    void setSlider(int16_t value)   { setAxis(6, value); }
    void setDial(int16_t value)     { setAxis(7, value); }
    
    // Hat switch methods (0-3), direction 0-8 or -1 for center
    void setHat(uint8_t hat, int8_t direction);
    
    // Report management
    bool sendReport();
    void sendState() { sendReport(); }
    bool isReady() const;
    
    // Auto-send control
    void setAutoSend(bool auto_send) { _auto_send = auto_send; }
    bool getAutoSend() const { return _auto_send; }
    
    // Performance monitoring
    uint32_t getLastSendTime() const { return _last_send_time; }
    
    // Reset all inputs
    void reset();
    
    // Feature report support for configuration protocol
    static void setFeatureReportCallback(uint16_t (*get_callback)(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen), 
                                        void (*set_callback)(uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize));
    
private:
    void _updateStateChanged();
    bool _canSend() const;
    void _sendIfChanged();
    
    // Feature report callbacks
    static uint16_t (*_get_feature_callback)(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
    static void (*_set_feature_callback)(uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize);
};

// Global instance for compatibility
extern TinyUSBGamepad MyGamepad;