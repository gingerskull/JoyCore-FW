// SPDX-License-Identifier: GPL-3.0-or-later
#include "TinyUSBGamepad.h"
#include <string.h>

// Custom HID descriptor for 128 buttons, 16 axes, 4 hat switches
const uint8_t TinyUSBGamepad::_hid_descriptor[] = {
    // Header
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    // REPORT_ID (1)
    
    // 128 Buttons
    0x05, 0x09,                    // USAGE_PAGE (Button)
    0x19, 0x01,                    // USAGE_MINIMUM (Button 1)
    0x29, 0x80,                    // USAGE_MAXIMUM (Button 128)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
    0x95, 0x80,                    // REPORT_COUNT (128)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    
    // 16 Axes (16-bit signed)
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    // USAGE (X)
    0x09, 0x31,                    // USAGE (Y)
    0x09, 0x32,                    // USAGE (Z)
    0x09, 0x33,                    // USAGE (Rx)
    0x09, 0x34,                    // USAGE (Ry)
    0x09, 0x35,                    // USAGE (Rz)
    0x09, 0x36,                    // USAGE (Slider)
    0x09, 0x37,                    // USAGE (Dial)
    0x09, 0x38,                    // USAGE (Wheel)
    0x09, 0x39,                    // USAGE (Hat switch) - reused as extra axis
    0x09, 0x3A,                    // USAGE (Counted Buffer)
    0x09, 0x3B,                    // USAGE (Byte Count)
    0x09, 0x3C,                    // USAGE (Motion Wakeup)
    0x09, 0x3D,                    // USAGE (Start)
    0x09, 0x3E,                    // USAGE (Select)
    0x09, 0x3F,                    // USAGE (Undefined)
    0x16, 0x01, 0x80,              // LOGICAL_MINIMUM (-32767)
    0x26, 0xFF, 0x7F,              // LOGICAL_MAXIMUM (32767)
    0x95, 0x10,                    // REPORT_COUNT (16)
    0x75, 0x10,                    // REPORT_SIZE (16)
    0x81, 0x02,                    // INPUT (Data,Var,Abs)
    
    // Hat switches temporarily removed due to phantom input issues
    // TODO: Re-enable when hat switch phantom issues are resolved
    
    0xc0                           // END_COLLECTION
};

const uint16_t TinyUSBGamepad::_hid_descriptor_len = sizeof(_hid_descriptor);

// Global instance
TinyUSBGamepad MyGamepad;

// Static callback pointers
uint16_t (*TinyUSBGamepad::_get_feature_callback)(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) = nullptr;
void (*TinyUSBGamepad::_set_feature_callback)(uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize) = nullptr;

TinyUSBGamepad::TinyUSBGamepad() : _auto_send(true), _last_send_time(0), _state_changed(false), _hat_switches_disabled(true) {
    // Initialize report structures
    memset(&_report, 0, sizeof(_report));
    memset(&_prev_report, 0, sizeof(_prev_report));
    
    // Hat switches removed from report structure - no initialization needed
}

TinyUSBGamepad::~TinyUSBGamepad() {
    end();
}

bool TinyUSBGamepad::begin(bool auto_send) {
    _auto_send = auto_send;
    
    // Configure HID device
    _usb_hid.setPollInterval(1); // 1ms polling = 1000Hz
    _usb_hid.setReportDescriptor(_hid_descriptor, _hid_descriptor_len);
    
    // Set up TinyUSB callbacks for feature reports
    _usb_hid.setReportCallback(
        [](uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) -> uint16_t {
            // GET_REPORT callback
            if (_get_feature_callback) {
                return _get_feature_callback(report_id, report_type, buffer, reqlen);
            }
            return 0;
        },
        [](uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize) {
            // SET_REPORT callback  
            if (_set_feature_callback) {
                _set_feature_callback(report_id, report_type, buffer, bufsize);
            }
        }
    );
    
    // Start HID
    if (!_usb_hid.begin()) {
        return false;
    }
    
    // Wait for USB to be ready
    delay(100);
    
    return true;
}

void TinyUSBGamepad::end() {
    // Reset all inputs before ending
    reset();
    sendReport();
}

void TinyUSBGamepad::setButton(uint8_t button, bool pressed) {
    if (button >= 128) return;
    
    uint8_t byte_idx = button / 8;
    uint8_t bit_idx = button % 8;
    
    if (pressed) {
        _report.buttons[byte_idx] |= (1 << bit_idx);
    } else {
        _report.buttons[byte_idx] &= ~(1 << bit_idx);
    }
    
    _updateStateChanged();
    _sendIfChanged();
}

void TinyUSBGamepad::pressButton(uint8_t button) {
    setButton(button, true);
}

void TinyUSBGamepad::releaseButton(uint8_t button) {
    setButton(button, false);
}

void TinyUSBGamepad::releaseAllButtons() {
    memset(_report.buttons, 0, sizeof(_report.buttons));
    _updateStateChanged();
    _sendIfChanged();
}

void TinyUSBGamepad::setAxis(uint8_t axis, int16_t value) {
    if (axis >= 16) return;
    
    // Clamp value to valid range
    if (value < -32767) value = -32767;
    if (value > 32767) value = 32767;
    
    _report.axes[axis] = value;
    _updateStateChanged();
    _sendIfChanged();
}

void TinyUSBGamepad::setHat(uint8_t hat, int8_t direction) {
    // Hat switches completely disabled - removed from HID descriptor and report structure
    // This method is kept for API compatibility but does nothing
    (void)hat;      // Suppress unused parameter warning
    (void)direction; // Suppress unused parameter warning
    return;
}

bool TinyUSBGamepad::sendReport() {
    if (!isReady()) {
        return false;
    }
    
    if (!_canSend()) {
        return false;
    }
    
    bool success = _usb_hid.sendReport(1, &_report, sizeof(_report));
    
    if (success) {
        _last_send_time = micros();
        memcpy(&_prev_report, &_report, sizeof(_report));
        _state_changed = false;
    }
    
    return success;
}

bool TinyUSBGamepad::isReady() const {
    return const_cast<Adafruit_USBD_HID&>(_usb_hid).ready();
}

void TinyUSBGamepad::reset() {
    memset(&_report, 0, sizeof(_report));
    
    // Hat switches removed from report structure - no reset needed
    
    _updateStateChanged();
}

void TinyUSBGamepad::_updateStateChanged() {
    _state_changed = (memcmp(&_report, &_prev_report, sizeof(_report)) != 0);
}

bool TinyUSBGamepad::_canSend() const {
    uint32_t now = micros();
    return (now - _last_send_time) >= MIN_SEND_INTERVAL_US;
}

void TinyUSBGamepad::_sendIfChanged() {
    if (_auto_send && _state_changed) {
        sendReport();
    }
}

void TinyUSBGamepad::setFeatureReportCallback(uint16_t (*get_callback)(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen), 
                                             void (*set_callback)(uint8_t report_id, hid_report_type_t report_type, const uint8_t* buffer, uint16_t bufsize)) {
    _get_feature_callback = get_callback;
    _set_feature_callback = set_callback;
}