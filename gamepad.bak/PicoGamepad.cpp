/*
 * Copyright (c) 2018-2019, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "stdint.h"
#include "PluggableUSBHID.h"
#include "PicoGamepad.h"
#include "usb_phy_api.h"

#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_VOLUME 3

using namespace arduino;

//uint8_t inputArray[35];

PicoGamepad::PicoGamepad(bool connect, uint16_t vendor_id, uint16_t product_id, uint16_t product_release) : USBHID(get_usb_phy(), 0, 0, vendor_id, product_id, product_release)
{
    //_lock_status = 0;
    for (int i = 0; i < 4; i++)
    {
        SetHat(i, HAT_DIR_C);
    }
}

PicoGamepad::PicoGamepad(USBPhy *phy, uint16_t vendor_id, uint16_t product_id, uint16_t product_release) : USBHID(phy, 0, 0, vendor_id, product_id, product_release)
{
    //_lock_status = 0;
    for (int i = 0; i < 4; i++)
    {
        SetHat(i, HAT_DIR_C);
    }
    // User or child responsible for calling connect or init
}

PicoGamepad::~PicoGamepad()
{
    // for (int i = 0; i < 35; i++)
    // {
    //     inputArray[i] = 0;
    // }
    for (int i = 0; i < 4; i++)
    {
        SetHat(i, HAT_DIR_C);
    }
}

const uint8_t *PicoGamepad::report_desc()
{
    static const uint8_t reportDescriptor[] = {
        // HEADER
        0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
        0x09, 0x04,         // USAGE (Gamepad)
        0xA1, 0x01,         // COLLECTION (Application)
        0x85, 0x01,         // REPORT_ID (1)

        // BUTTONS (128 buttons)
        0x05, 0x09,         // USAGE_PAGE (Button)
        0x19, 0x01,         // USAGE_MINIMUM (Button 1)
        0x29, 0x80,         // USAGE_MAXIMUM (Button 128)
        0x15, 0x00,         // LOGICAL_MINIMUM (0)
        0x25, 0x01,         // LOGICAL_MAXIMUM (1)
        0x75, 0x01,         // REPORT_SIZE (1)
        0x95, 0x80,         // REPORT_COUNT (128)
        0x81, 0x02,         // INPUT (Data,Var,Abs)

        // 16 ANALOG AXES (16-bit signed)
        0x05, 0x01,         // USAGE_PAGE (Generic Desktop)
        // First 8 axes (Standard game controls)
        0x09, 0x30,         // USAGE (X)
        0x09, 0x31,         // USAGE (Y)
        0x09, 0x32,         // USAGE (Z)
        0x09, 0x33,         // USAGE (Rx)
        0x09, 0x34,         // USAGE (Ry)
        0x09, 0x35,         // USAGE (Rz)
        0x09, 0x36,         // USAGE (Slider)
        0x09, 0x37,         // USAGE (Dial)
        // Next 8 axes (Extended analog controls)
        0x09, 0x38,         // USAGE (Wheel)
        0x09, 0x39,         // USAGE (Vx - Velocity X)
        0x09, 0x41,         // USAGE (Vy - Velocity Y)
        0x09, 0x42,         // USAGE (Vz - Velocity Z)
        0x09, 0x43,         // USAGE (Vbrx - Angular Velocity X)
        0x09, 0x44,         // USAGE (Vbry - Angular Velocity Y)
        0x09, 0x45,         // USAGE (Vbrz - Angular Velocity Z)
        0x09, 0x46,         // USAGE (Vno - Velocity Norm)
        0x09, 0x47,         // USAGE (System Undefined - Free to use)
        
        0x16, 0x01, 0x80,   // LOGICAL_MINIMUM (-32767)
        0x26, 0xFF, 0x7F,   // LOGICAL_MAXIMUM (32767)
        0x75, 0x10,         // REPORT_SIZE (16)
        0x95, 0x10,         // REPORT_COUNT (16)  // 16 axes!
        0x81, 0x02,         // INPUT (Data,Var,Abs)

        // HAT SWITCHES (4-way digital, 4 switches)
        0x09, 0x39,         // USAGE (Hat switch)
        0x15, 0x00,         // LOGICAL_MINIMUM (0)
        0x25, 0x07,         // LOGICAL_MAXIMUM (7)
        0x35, 0x00,         // PHYSICAL_MINIMUM (0)
        0x46, 0x38, 0x01,   // PHYSICAL_MAXIMUM (315)
        0x65, 0x14,         // UNIT (Eng Rot:Angular Pos)
        0x75, 0x04,         // REPORT_SIZE (4)
        0x95, 0x01,         // REPORT_COUNT (1)
        0x81, 0x02,         // INPUT (Data,Var,Abs)

        // Repeat hat switch 3 more times (total 4)
        0x09, 0x39, 0x81, 0x02,  // Hat switch 2
        0x09, 0x39, 0x81, 0x02,  // Hat switch 3
        0x09, 0x39, 0x81, 0x02,  // Hat switch 4

        0xC0                // END_COLLECTION
    };
    reportLength = sizeof(reportDescriptor);
    return reportDescriptor;
}

bool PicoGamepad::randomizeInputs()
{
    _mutex.lock();

    HID_REPORT report;
    report.data[0] = 0x01;
    for (int i = 1; i < 51; i++)
    {
        report.data[i] = random();
    }
    report.length = 51;

    if (!send(&report))
    {
        _mutex.unlock();
        return false;
    }

    _mutex.unlock();
    return true;
}

void PicoGamepad::SetButton(int idx, bool val)
{
    if (idx > 128 || idx < 0)
    {
        return;
    }
    bitWrite(inputArray[idx / 8], idx % 8, val);
}

void PicoGamepad::SetAxis(int idx, uint16_t val)
{
    if (idx < 0 || idx > 15)
    {
        return;
    }
    if (idx > 9 && idx < 16){
        idx = 25 - idx;
    }

    inputArray[16 + (idx * 2)] = LSB(val);
    inputArray[16 + (idx * 2)+1] = MSB(val);
}

void PicoGamepad::SetX(uint16_t val)
{
    inputArray[X_AXIS_LSB] = LSB(val);
    inputArray[X_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetY(uint16_t val)
{
    inputArray[Y_AXIS_LSB] = LSB(val);
    inputArray[Y_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetZ(uint16_t val)
{
    inputArray[Z_AXIS_LSB] = LSB(val);
    inputArray[Z_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetRx(uint16_t val)
{
    inputArray[Rx_AXIS_LSB] = LSB(val);
    inputArray[Rx_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetRy(uint16_t val)
{
    inputArray[Ry_AXIS_LSB] = LSB(val);
    inputArray[Ry_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetRz(uint16_t val)
{
    inputArray[Rz_AXIS_LSB] = LSB(val);
    inputArray[Rz_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetSlider(uint16_t val)
{
    inputArray[SLIDER_AXIS_LSB] = LSB(val);
    inputArray[SLIDER_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetDial(uint16_t val)
{
    inputArray[DIAL_AXIS_LSB] = LSB(val);
    inputArray[DIAL_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetWheel(uint16_t val)
{
    inputArray[WHEEL_AXIS_LSB] = LSB(val);
    inputArray[WHEEL_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVx(uint16_t val)
{
    inputArray[Vx_AXIS_LSB] = LSB(val);
    inputArray[Vx_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVy(uint16_t val)
{
    inputArray[Vy_AXIS_LSB] = LSB(val);
    inputArray[Vy_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVz(uint16_t val)
{
    inputArray[Vz_AXIS_LSB] = LSB(val);
    inputArray[Vz_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVbrx(uint16_t val)
{
    inputArray[Vbrx_AXIS_LSB] = LSB(val);
    inputArray[Vbrx_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVbry(uint16_t val)
{
    inputArray[Vbry_AXIS_LSB] = LSB(val);
    inputArray[Vbry_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVbrz(uint16_t val)
{
    inputArray[Vbrz_AXIS_LSB] = LSB(val);
    inputArray[Vbrz_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetVno(uint16_t val)
{
    inputArray[Vno_AXIS_LSB] = LSB(val);
    inputArray[Vno_AXIS_MSB] = MSB(val);
}

void PicoGamepad::SetUndefined(uint16_t val)
{
    inputArray[UNDEF_AXIS_LSB] = LSB(val);
    inputArray[UNDEF_AXIS_MSB] = MSB(val);
}


void PicoGamepad::SetHat(uint8_t hatIdx, uint8_t dir)
{
    uint8_t hatDir[9][4] = {
        {0, 0, 0, 0},
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 1, 0, 0},
        {0, 1, 0, 1},
        {0, 1, 1, 0},
        {0, 1, 1, 1},
        {1, 0, 0, 0}};
    switch (hatIdx)
    {
    case 0:
        for (int i = 0; i < 4; i++)
        {
            bitWrite(inputArray[HAT0_1], 3 - i, hatDir[dir][i]);
        }
        break;
    case 1:
        for (int i = 0; i < 4; i++)
        {
            bitWrite(inputArray[HAT0_1], 7 - i, hatDir[dir][i]);
        }
        break;
    case 2:
        for (int i = 0; i < 4; i++)
        {
            bitWrite(inputArray[HAT2_3], 3 - i, hatDir[dir][i]);
        }
        break;
    case 3:
        for (int i = 0; i < 4; i++)
        {
            bitWrite(inputArray[HAT2_3], 7 - i, hatDir[dir][i]);
        }
        break;
    }
}

bool PicoGamepad::send_update()
{
    _mutex.lock();

    HID_REPORT report;
    report.data[0] = 0x01;
    for (int i = 1; i < 51; i++)
    {
        report.data[i] = inputArray[i - 1];
    }

    report.length = 51;

    if (!send(&report))
    {
        _mutex.unlock();
        return false;
    }

    _mutex.unlock();
    return true;
}

bool PicoGamepad::send_inputs(uint8_t *values)
{
    _mutex.lock();

    HID_REPORT report;
    report.data[0] = 0x01;
    for (int i = 1; i < 51; i++)
    {
        report.data[i] = values[i - 1];
    }

    report.length = 51;

    if (!send(&report))
    {
        _mutex.unlock();
        return false;
    }

    _mutex.unlock();
    return true;
}



#define DEFAULT_CONFIGURATION (1)
#define TOTAL_DESCRIPTOR_LENGTH ((1 * CONFIGURATION_DESCRIPTOR_LENGTH) + (1 * INTERFACE_DESCRIPTOR_LENGTH) + (1 * HID_DESCRIPTOR_LENGTH) + (2 * ENDPOINT_DESCRIPTOR_LENGTH))

const uint8_t *PicoGamepad::configuration_desc(uint8_t index)
{
    if (index != 0)
    {
        return NULL;
    }
    uint8_t configuration_descriptor_temp[] = {
        CONFIGURATION_DESCRIPTOR_LENGTH, // bLength
        CONFIGURATION_DESCRIPTOR,        // bDescriptorType
        LSB(TOTAL_DESCRIPTOR_LENGTH),    // wTotalLength (LSB)
        MSB(TOTAL_DESCRIPTOR_LENGTH),    // wTotalLength (MSB)
        0x01,                            // bNumInterfaces
        DEFAULT_CONFIGURATION,           // bConfigurationValue
        0x00,                            // iConfiguration
        C_RESERVED | C_SELF_POWERED,     // bmAttributes
        C_POWER(0),                      // bMaxPower

        INTERFACE_DESCRIPTOR_LENGTH, // bLength
        INTERFACE_DESCRIPTOR,        // bDescriptorType
        0x00,                        // bInterfaceNumber
        0x00,                        // bAlternateSetting
        0x02,                        // bNumEndpoints
        HID_CLASS,                   // bInterfaceClass
        HID_SUBCLASS_BOOT,           // bInterfaceSubClass
        HID_PROTOCOL_KEYBOARD,       // bInterfaceProtocol
        0x00,                        // iInterface

        HID_DESCRIPTOR_LENGTH,                // bLength
        HID_DESCRIPTOR,                       // bDescriptorType
        LSB(HID_VERSION_1_11),                // bcdHID (LSB)
        MSB(HID_VERSION_1_11),                // bcdHID (MSB)
        0x00,                                 // bCountryCode
        0x01,                                 // bNumDescriptors
        REPORT_DESCRIPTOR,                    // bDescriptorType
        (uint8_t)(LSB(report_desc_length())), // wDescriptorLength (LSB)
        (uint8_t)(MSB(report_desc_length())), // wDescriptorLength (MSB)

        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
        ENDPOINT_DESCRIPTOR,        // bDescriptorType
        _int_in,                    // bEndpointAddress
        E_INTERRUPT,                // bmAttributes
        LSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (LSB)
        MSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (MSB)
        1,                          // bInterval (milliseconds)

        ENDPOINT_DESCRIPTOR_LENGTH, // bLength
        ENDPOINT_DESCRIPTOR,        // bDescriptorType
        _int_out,                   // bEndpointAddress
        E_INTERRUPT,                // bmAttributes
        LSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (LSB)
        MSB(MAX_HID_REPORT_SIZE),   // wMaxPacketSize (MSB)
        1,                          // bInterval (milliseconds)
    };
    MBED_ASSERT(sizeof(configuration_descriptor_temp) == sizeof(_configuration_descriptor));
    memcpy(_configuration_descriptor, configuration_descriptor_temp, sizeof(_configuration_descriptor));
    return _configuration_descriptor;
}
