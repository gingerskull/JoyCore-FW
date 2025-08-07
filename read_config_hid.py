#!/usr/bin/env python3
"""
Read JoyCore configuration via USB HID protocol

This script communicates with the JoyCore firmware using the USB HID
configuration protocol to read the stored configuration directly.
"""

import hid
import struct
import time
import sys

# Configuration constants from firmware
CONFIG_USB_FEATURE_REPORT_ID = 0x02
CONFIG_USB_MAX_PACKET_SIZE = 64

# Message types from ConfigStructs.h
class ConfigMessageType:
    GET_CONFIG = 0x01
    SET_CONFIG = 0x02
    RESET_CONFIG = 0x03
    VALIDATE_CONFIG = 0x04
    GET_CONFIG_STATUS = 0x05
    SAVE_CONFIG = 0x06
    LOAD_CONFIG = 0x07

def find_joycore_device():
    """Find JoyCore device by VID/PID"""
    # Default VID/PID from ConfigDigital.h
    VID = 0x2E8A  # Raspberry Pi Foundation
    PID = 0xA02F  # JoyCore custom PID
    
    devices = hid.enumerate()
    joycore_devices = []
    
    for device in devices:
        if device['vendor_id'] == VID and device['product_id'] == PID:
            joycore_devices.append(device)
            print(f"Found device on interface: {device.get('interface_number', 'N/A')}")
    
    # Try to find the gamepad interface (usually 0 or 2)
    for device in joycore_devices:
        if device.get('usage_page') == 0x01 and device.get('usage') == 0x05:  # Game pad
            return device
    
    # If no gamepad interface found, return first one
    return joycore_devices[0] if joycore_devices else None

def create_config_message(msg_type, sequence=0, total_packets=1, data=b''):
    """Create a ConfigMessage structure"""
    # ConfigMessage struct:
    # uint8_t reportID;
    # uint8_t type;
    # uint8_t sequence;
    # uint8_t totalPackets;
    # uint16_t dataLength;
    # uint8_t status;
    # uint8_t reserved;
    # uint8_t data[56];  # CONFIG_USB_MAX_PACKET_SIZE - 8
    
    message = bytearray(CONFIG_USB_MAX_PACKET_SIZE)
    message[0] = CONFIG_USB_FEATURE_REPORT_ID
    message[1] = msg_type
    message[2] = sequence
    message[3] = total_packets
    struct.pack_into('<H', message, 4, len(data))  # dataLength
    message[6] = 0  # status
    message[7] = 0  # reserved
    
    if data:
        message[8:8+len(data)] = data
    
    return bytes(message)

def parse_config_status(data):
    """Parse ConfigStatus structure"""
    # ConfigStatus struct:
    # bool storageInitialized;
    # bool configLoaded;
    # bool usingDefaults;
    # uint8_t currentMode;
    # uint32_t storageUsed;
    # uint32_t storageAvailable;
    # uint16_t configVersion;
    # uint8_t reserved[6];
    
    if len(data) < 20:
        return None
    
    status = {
        'storageInitialized': bool(data[0]),
        'configLoaded': bool(data[1]),
        'usingDefaults': bool(data[2]),
        'currentMode': data[3],
        'storageUsed': struct.unpack('<I', data[4:8])[0],
        'storageAvailable': struct.unpack('<I', data[8:12])[0],
        'configVersion': struct.unpack('<H', data[12:14])[0]
    }
    
    mode_names = {0: 'STATIC', 1: 'STORAGE', 2: 'HYBRID'}
    status['modeName'] = mode_names.get(status['currentMode'], 'UNKNOWN')
    
    return status

def parse_config_header(data):
    """Parse ConfigHeader structure"""
    if len(data) < 16:
        return None
    
    header = {
        'magic': struct.unpack('<I', data[0:4])[0],
        'version': struct.unpack('<H', data[4:6])[0],
        'size': struct.unpack('<H', data[6:8])[0],
        'checksum': struct.unpack('<I', data[8:12])[0]
    }
    
    # Convert magic to string
    header['magic_str'] = ''.join(chr((header['magic'] >> (8*i)) & 0xFF) for i in range(4))
    
    return header

def parse_usb_descriptor(data):
    """Parse StoredUSBDescriptor"""
    if len(data) < 76:
        return None
    
    desc = {
        'vendorID': struct.unpack('<H', data[0:2])[0],
        'productID': struct.unpack('<H', data[2:4])[0],
        'manufacturer': data[4:36].decode('utf-8', errors='ignore').rstrip('\x00'),
        'product': data[36:68].decode('utf-8', errors='ignore').rstrip('\x00')
    }
    
    return desc

def parse_stored_config(data):
    """Parse complete StoredConfig structure"""
    offset = 0
    config = {}
    
    # Parse header
    config['header'] = parse_config_header(data[offset:])
    if not config['header']:
        return None
    offset += 16
    
    # Parse USB descriptor
    config['usb'] = parse_usb_descriptor(data[offset:])
    if not config['usb']:
        return None
    offset += 76
    
    # Parse counts
    if len(data) >= offset + 4:
        config['pinMapCount'] = data[offset]
        config['logicalInputCount'] = data[offset + 1]
        config['shiftRegCount'] = data[offset + 2]
        offset += 4
    
    # Parse axis configs (8 axes)
    config['axes'] = []
    for i in range(8):
        if len(data) >= offset + 16:
            axis = {
                'enabled': bool(data[offset]),
                'pin': data[offset + 1],
                'minValue': struct.unpack('<H', data[offset+2:offset+4])[0],
                'maxValue': struct.unpack('<H', data[offset+4:offset+6])[0],
                'filterLevel': data[offset + 6],
                'ewmaAlpha': data[offset + 7],
                'deadband': struct.unpack('<H', data[offset+8:offset+10])[0],
                'curve': data[offset + 10]
            }
            config['axes'].append(axis)
            offset += 16
    
    # Parse variable data (pin map and logical inputs)
    config['variable_data_offset'] = offset
    
    return config

def send_hid_command(device_path, msg_type):
    """Send HID command and get response"""
    try:
        # Open HID device
        device = hid.device()
        device.open_path(device_path)
        
        # Set non-blocking mode
        device.set_nonblocking(1)
        
        # Create and send command
        message = create_config_message(msg_type)
        
        print(f"Sending feature report with ID {CONFIG_USB_FEATURE_REPORT_ID}, type {msg_type}")
        
        # Try to send feature report
        try:
            bytes_written = device.send_feature_report(message)
            print(f"Sent {bytes_written} bytes")
        except Exception as e:
            print(f"Failed to send feature report: {e}")
            # Try writing directly
            bytes_written = device.write(message)
            print(f"Tried direct write: {bytes_written} bytes")
        
        # Wait a bit for processing
        time.sleep(0.2)
        
        # Try to read response
        try:
            response = device.get_feature_report(CONFIG_USB_FEATURE_REPORT_ID, CONFIG_USB_MAX_PACKET_SIZE)
            print(f"Received feature report: {len(response)} bytes")
        except Exception as e:
            print(f"Failed to get feature report: {e}")
            # Try reading directly - but this will get input reports, not feature reports
            response = device.read(CONFIG_USB_MAX_PACKET_SIZE, timeout_ms=1000)
            if response:
                print(f"Direct read got {len(response)} bytes (this is input report, not feature report!)")
                # Convert list to bytes for hex display
                resp_bytes = bytes(response) if isinstance(response, list) else response
                print(f"First bytes: {resp_bytes[:10].hex() if len(resp_bytes) >= 10 else resp_bytes.hex()}")
                # This is wrong data - it's the joystick input report
                response = None
            else:
                print("No data from direct read")
                response = None
        
        device.close()
        return bytes(response) if response else None
        
    except Exception as e:
        print(f"Error communicating with device: {e}")
        import traceback
        traceback.print_exc()
        return None

def main():
    # Find JoyCore device
    device_info = find_joycore_device()
    if not device_info:
        print("JoyCore device not found!")
        print("\nAvailable HID devices:")
        for dev in hid.enumerate():
            print(f"  VID={dev['vendor_id']:04X} PID={dev['product_id']:04X} - {dev['manufacturer_string']} {dev['product_string']}")
        return
    
    print(f"Found JoyCore device:")
    print(f"  Path: {device_info['path']}")
    print(f"  Manufacturer: {device_info['manufacturer_string']}")
    print(f"  Product: {device_info['product_string']}")
    print(f"  VID/PID: {device_info['vendor_id']:04X}:{device_info['product_id']:04X}")
    print(f"  Usage Page: {device_info.get('usage_page', 'N/A')}")
    print(f"  Usage: {device_info.get('usage', 'N/A')}")
    print(f"  Interface: {device_info.get('interface_number', 'N/A')}")
    
    # Get configuration status
    print("\n" + "="*60)
    print("Configuration Status:")
    print("="*60)
    
    response = send_hid_command(device_info['path'], ConfigMessageType.GET_CONFIG_STATUS)
    if response and len(response) >= 8:
        # Skip message header (8 bytes) to get to data
        status_data = response[8:]
        status = parse_config_status(status_data)
        if status:
            print(f"  Mode: {status['modeName']} ({status['currentMode']})")
            print(f"  Config Loaded: {status['configLoaded']}")
            print(f"  Using Defaults: {status['usingDefaults']}")
            print(f"  Storage Initialized: {status['storageInitialized']}")
            print(f"  Storage Used: {status['storageUsed']} bytes")
            print(f"  Storage Available: {status['storageAvailable']} bytes")
            print(f"  Config Version: {status['configVersion']}")
    
    # Get full configuration
    print("\n" + "="*60)
    print("Current Configuration:")
    print("="*60)
    
    response = send_hid_command(device_info['path'], ConfigMessageType.GET_CONFIG)
    if response and len(response) >= 8:
        # Check if we got data
        msg_type = response[1]
        status = response[6]
        data_length = struct.unpack('<H', response[4:6])[0]
        
        if status == 0 and data_length > 0:
            config_data = response[8:8+data_length]
            config = parse_stored_config(config_data)
            
            if config:
                print(f"\nConfig Header:")
                print(f"  Magic: 0x{config['header']['magic']:08X} ('{config['header']['magic_str']}')")
                print(f"  Version: {config['header']['version']}")
                print(f"  Size: {config['header']['size']} bytes")
                
                print(f"\nUSB Descriptor:")
                print(f"  VID: 0x{config['usb']['vendorID']:04X}")
                print(f"  PID: 0x{config['usb']['productID']:04X}")
                print(f"  Manufacturer: '{config['usb']['manufacturer']}'")
                print(f"  Product: '{config['usb']['product']}'")
                
                print(f"\nConfiguration Counts:")
                print(f"  Pin Map Entries: {config['pinMapCount']}")
                print(f"  Logical Inputs: {config['logicalInputCount']}")
                print(f"  Shift Registers: {config['shiftRegCount']}")
                
                print(f"\nAxis Configuration:")
                for i, axis in enumerate(config['axes']):
                    if axis['enabled']:
                        print(f"  Axis {i}: Pin={axis['pin']}, Range={axis['minValue']}-{axis['maxValue']}")
        else:
            print(f"  Error: Status={status}, Data Length={data_length}")

if __name__ == "__main__":
    main()