#!/usr/bin/env python3
"""
JoyCore Configuration Test Script
Tests the USB HID configuration protocol with the RP2040 board
"""

import hid
import struct
import time
import sys
import re
import os

# USB HID constants from src/config/core/ConfigMode.h
CONFIG_USB_FEATURE_REPORT_ID = 0x02
CONFIG_USB_MAX_PACKET_SIZE = 64

# Message types from src/config/core/ConfigStructs.h
class ConfigMessageType:
    GET_CONFIG = 0x01
    SET_CONFIG = 0x02
    RESET_CONFIG = 0x03
    VALIDATE_CONFIG = 0x04
    GET_CONFIG_STATUS = 0x05
    SAVE_CONFIG = 0x06
    LOAD_CONFIG = 0x07

# Configuration message structure
# struct ConfigMessage {
#     uint8_t reportID;        // HID Report ID (CONFIG_USB_FEATURE_REPORT_ID)
#     ConfigMessageType type;  // Message type
#     uint8_t sequence;        // Sequence number for multi-packet transfers
#     uint8_t totalPackets;    // Total number of packets in transfer
#     uint16_t dataLength;     // Length of data in this packet
#     uint8_t status;          // Status/error code
#     uint8_t reserved;        // Padding
#     uint8_t data[CONFIG_USB_MAX_PACKET_SIZE - 8]; // Payload data
# } __attribute__((packed));

def find_joycore_device():
    """Find the JoyCore device by looking for our custom HID interface"""
    print("Scanning for JoyCore devices...")
    
    devices = hid.enumerate()
    
    # First try our custom VID:PID
    for device in devices:
        if device['vendor_id'] == 0x1235 and device['product_id'] == 0x0050:
            print(f"Found JoyCore device (custom): {device['product_string']}")
            print(f"  Manufacturer: {device['manufacturer_string']}")
            print(f"  Path: {device['path']}")
            print(f"  Interface: {device['interface_number']}")
            return device['path']
    
    # If not found, try the default RP2040 Pico VID:PID
    for device in devices:
        if device['vendor_id'] == 0x2e8a and device['product_id'] == 0x00c0:
            print(f"Found RP2040 Pico device: {device['product_string']}")
            print(f"  Manufacturer: {device['manufacturer_string']}")
            print(f"  Path: {device['path']}")
            print(f"  Interface: {device['interface_number']}")
            # Try to use the first interface that supports feature reports
            return device['path']
    
    print("No JoyCore device found!")
    print("Available HID devices:")
    for device in devices:
        print(f"  VID:PID {device['vendor_id']:04x}:{device['product_id']:04x} - {device['product_string']}")
    
    return None

def create_config_message(msg_type, data=b''):
    """Create a configuration message"""
    message = bytearray(CONFIG_USB_MAX_PACKET_SIZE)
    
    # Pack the message header
    message[0] = CONFIG_USB_FEATURE_REPORT_ID  # reportID
    message[1] = msg_type                      # type
    message[2] = 0                             # sequence
    message[3] = 1                             # totalPackets
    
    # dataLength (little-endian uint16)
    data_len = len(data)
    message[4] = data_len & 0xFF
    message[5] = (data_len >> 8) & 0xFF
    
    message[6] = 0                             # status
    message[7] = 0                             # reserved
    
    # Copy data payload
    if data:
        message[8:8+len(data)] = data
    
    return bytes(message)

def parse_config_response(response):
    """Parse a configuration response message"""
    if len(response) < 8:
        return None
    
    report_id = response[0]
    msg_type = response[1]
    sequence = response[2]
    total_packets = response[3]
    data_length = response[4] | (response[5] << 8)
    status = response[6]
    reserved = response[7]
    data = response[8:8+data_length] if data_length > 0 else b''
    
    return {
        'report_id': report_id,
        'type': msg_type,
        'sequence': sequence,
        'total_packets': total_packets,
        'data_length': data_length,
        'status': status,
        'data': data
    }

def test_get_status(device):
    """Test GET_STATUS command"""
    print("\n=== Testing GET_STATUS ===")
    
    try:
        # Create GET_STATUS message
        message = create_config_message(ConfigMessageType.GET_CONFIG_STATUS)
        print(f"Sending GET_STATUS message: {message[:16].hex()}...")
        
        # Send feature report
        print(f"Sending feature report with {len(message)} bytes")
        result = device.send_feature_report(message)
        print(f"Send result: {result}")
        
        # Small delay to allow processing
        time.sleep(0.1)
        
        # Read response
        print(f"Requesting feature report {CONFIG_USB_FEATURE_REPORT_ID} with max length {CONFIG_USB_MAX_PACKET_SIZE}")
        response = device.get_feature_report(CONFIG_USB_FEATURE_REPORT_ID, CONFIG_USB_MAX_PACKET_SIZE)
        print(f"Got response of length: {len(response) if response else 'None'}")
        print(f"Received response: {response[:16].hex()}...")
        
        # Parse response
        parsed = parse_config_response(response)
        if parsed:
            print(f"Response type: {parsed['type']}")
            print(f"Status code: {parsed['status']} ({'SUCCESS' if parsed['status'] == 0 else 'ERROR'})")
            print(f"Data length: {parsed['data_length']}")
            
            if parsed['data_length'] > 0:
                print(f"Status data: {parsed['data'].hex()}")
                
                # Try to parse ConfigStatus structure
                if parsed['data_length'] >= 16:  # Minimum expected size
                    data = parsed['data']
                    storage_initialized = bool(data[0])
                    config_loaded = bool(data[1]) 
                    using_defaults = bool(data[2])
                    current_mode = data[3]
                    storage_used = struct.unpack('<I', data[4:8])[0]
                    storage_available = struct.unpack('<I', data[8:12])[0]
                    config_version = struct.unpack('<H', data[12:14])[0]
                    
                    print(f"  Storage initialized: {storage_initialized}")
                    print(f"  Config loaded: {config_loaded}")
                    print(f"  Using defaults: {using_defaults}")
                    print(f"  Current mode: {current_mode}")
                    print(f"  Storage used: {storage_used} bytes")
                    print(f"  Storage available: {storage_available} bytes")
                    print(f"  Config version: {config_version}")
        
        return True
        
    except Exception as e:
        print(f"Error testing GET_STATUS: {e}")
        return False

def parse_button_behavior(behavior_id):
    """Convert behavior ID to human-readable string"""
    behaviors = {
        0: "NORMAL",
        1: "MOMENTARY", 
        2: "ENC_A",
        3: "ENC_B"
    }
    return behaviors.get(behavior_id, f"UNKNOWN({behavior_id})")

def parse_input_type(input_type):
    """Convert input type ID to human-readable string"""
    types = {
        0: "INPUT_PIN",
        1: "INPUT_MATRIX",
        2: "INPUT_SHIFTREG"
    }
    return types.get(input_type, f"UNKNOWN({input_type})")

def parse_latch_mode(latch_mode):
    """Convert latch mode ID to human-readable string"""
    modes = {
        0: "FOUR0",
        1: "FOUR1", 
        2: "FOUR2",
        3: "FOUR3"
    }
    return modes.get(latch_mode, f"UNKNOWN({latch_mode})")

def parse_config_data(config_data):
    """Parse configuration data and extract button settings"""
    print("\n=== Parsing Configuration Data ===")
    
    if not config_data or len(config_data) < 8:
        print("Insufficient configuration data")
        return
        
    try:
        # Configuration data structure (based on src/config/core/ConfigStructs.h):
        # struct Config {
        #     uint32_t magic;           // Magic number to validate config
        #     uint16_t version;         // Config version
        #     uint16_t size;            // Total config size
        #     DigitalConfig digital;    // Digital input configuration
        #     AnalogConfig analog;      // Analog axis configuration
        # }
        
        offset = 0
        
        # Parse main config header
        magic = struct.unpack('<I', config_data[offset:offset+4])[0]
        offset += 4
        
        version = struct.unpack('<H', config_data[offset:offset+2])[0]
        offset += 2
        
        size = struct.unpack('<H', config_data[offset:offset+2])[0]
        offset += 2
        
        print(f"Config Magic: 0x{magic:08X}")
        print(f"Config Version: {version}")
        print(f"Config Size: {size} bytes")
        
        if len(config_data) < size:
            print(f"Warning: Received {len(config_data)} bytes but config size is {size}")
        
        # Parse digital configuration
        if offset + 4 <= len(config_data):
            digital_input_count = struct.unpack('<H', config_data[offset:offset+2])[0]
            offset += 2
            
            shiftreg_count = struct.unpack('<H', config_data[offset:offset+2])[0]
            offset += 2
            
            print(f"\nDigital Configuration:")
            print(f"  Input count: {digital_input_count}")
            print(f"  Shift register count: {shiftreg_count}")
            
            # Parse logical inputs
            print(f"\n=== Button Configuration ===")
            for i in range(min(digital_input_count, 50)):  # Limit to prevent overflow
                if offset + 12 > len(config_data):  # LogicalInput is ~12 bytes
                    break
                    
                input_type = config_data[offset]
                offset += 1
                
                # Parse input data union (8 bytes)
                input_data = config_data[offset:offset+8]
                offset += 8
                
                # Parse latch mode (4 bytes, but only 1 byte used)
                latch_mode = config_data[offset]
                offset += 4  # Skip padding
                
                print(f"  Input {i+1}:")
                print(f"    Type: {parse_input_type(input_type)}")
                print(f"    Latch Mode: {parse_latch_mode(latch_mode)}")
                
                # Parse specific input data based on type
                if input_type == 0:  # INPUT_PIN
                    pin = input_data[0]
                    button_id = input_data[1]
                    behavior = input_data[2]
                    reversed_pin = input_data[3]
                    print(f"    Pin: {pin}")
                    print(f"    Button ID: {button_id}")
                    print(f"    Behavior: {parse_button_behavior(behavior)}")
                    print(f"    Reversed: {bool(reversed_pin)}")
                    
                elif input_type == 1:  # INPUT_MATRIX
                    row = input_data[0]
                    col = input_data[1]
                    button_id = input_data[2]
                    behavior = input_data[3]
                    reversed_pin = input_data[4]
                    print(f"    Matrix Position: Row {row}, Col {col}")
                    print(f"    Button ID: {button_id}")
                    print(f"    Behavior: {parse_button_behavior(behavior)}")
                    print(f"    Reversed: {bool(reversed_pin)}")
                    
                elif input_type == 2:  # INPUT_SHIFTREG
                    register = input_data[0]
                    bit = input_data[1]
                    button_id = input_data[2]
                    behavior = input_data[3]
                    reversed_pin = input_data[4]
                    print(f"    Shift Register: {register}, Bit: {bit}")
                    print(f"    Button ID: {button_id}")
                    print(f"    Behavior: {parse_button_behavior(behavior)}")
                    print(f"    Reversed: {bool(reversed_pin)}")
                
                print()
                
        # Try to parse analog config if there's more data
        if offset < len(config_data):
            print(f"\n=== Analog Configuration ===")
            print(f"Remaining data: {len(config_data) - offset} bytes")
            print(f"Analog config data: {config_data[offset:offset+16].hex()}...")
            
    except Exception as e:
        print(f"Error parsing config data: {e}")
        print(f"Raw data: {config_data[:64].hex()}...")

def test_get_config(device):
    """Test GET_CONFIG command and parse button settings"""
    print("\n=== Testing GET_CONFIG ===")
    
    try:
        # Create GET_CONFIG message
        message = create_config_message(ConfigMessageType.GET_CONFIG)
        print(f"Sending GET_CONFIG message: {message[:16].hex()}...")
        
        # Send feature report
        print(f"Sending feature report with {len(message)} bytes")
        result = device.send_feature_report(message)
        print(f"Send result: {result}")
        
        # Small delay to allow processing
        time.sleep(0.1)
        
        # Read response
        print(f"Requesting feature report {CONFIG_USB_FEATURE_REPORT_ID} with max length {CONFIG_USB_MAX_PACKET_SIZE}")
        response = device.get_feature_report(CONFIG_USB_FEATURE_REPORT_ID, CONFIG_USB_MAX_PACKET_SIZE)
        print(f"Got response of length: {len(response) if response else 'None'}")
        print(f"Received response: {response[:16].hex()}...")
        
        # Parse response
        parsed = parse_config_response(response)
        if parsed:
            print(f"Response type: {parsed['type']}")
            print(f"Status code: {parsed['status']} ({'SUCCESS' if parsed['status'] == 0 else 'ERROR'})")
            print(f"Data length: {parsed['data_length']}")
            
            if parsed['data_length'] > 0:
                print(f"Config data preview: {parsed['data'][:32].hex()}...")
                print(f"Total config size: {len(parsed['data'])} bytes")
                
                # Parse the configuration data to show button settings
                parse_config_data(parsed['data'])
        
        return True
        
    except Exception as e:
        print(f"Error testing GET_CONFIG: {e}")
        return False

def create_button_summary(device):
    """Create a summary table of all configured buttons"""
    print("\n=== Button Configuration Summary ===")
    
    try:
        # Get configuration from device
        message = create_config_message(ConfigMessageType.GET_CONFIG)
        device.send_feature_report(message)
        time.sleep(0.1)
        response = device.get_feature_report(CONFIG_USB_FEATURE_REPORT_ID, CONFIG_USB_MAX_PACKET_SIZE)
        parsed = parse_config_response(response)
        
        if not parsed or parsed['data_length'] <= 0:
            print("Could not retrieve configuration from device")
            return
        
        # Parse the config to extract button mappings
        config_data = parsed['data']
        buttons = {}  # button_id -> details
        
        # Quick parse to extract button info
        offset = 8  # Skip header
        if offset + 4 <= len(config_data):
            digital_input_count = struct.unpack('<H', config_data[offset:offset+2])[0]
            offset += 4  # Skip shiftreg count too
            
            for i in range(min(digital_input_count, 50)):
                if offset + 12 > len(config_data):
                    break
                    
                input_type = config_data[offset]
                offset += 1
                input_data = config_data[offset:offset+8]
                offset += 8
                latch_mode = config_data[offset]
                offset += 4
                
                if input_type == 0:  # INPUT_PIN
                    pin, button_id, behavior, reversed_pin = input_data[:4]
                    buttons[button_id] = {
                        'source': f'Pin {pin}',
                        'behavior': parse_button_behavior(behavior),
                        'type': 'Direct Pin',
                        'reversed': bool(reversed_pin)
                    }
                elif input_type == 1:  # INPUT_MATRIX
                    row, col, button_id, behavior, reversed_pin = input_data[:5]
                    buttons[button_id] = {
                        'source': f'Matrix[{row},{col}]',
                        'behavior': parse_button_behavior(behavior),
                        'type': 'Button Matrix',
                        'reversed': bool(reversed_pin)
                    }
                elif input_type == 2:  # INPUT_SHIFTREG
                    register, bit, button_id, behavior, reversed_pin = input_data[:5]
                    buttons[button_id] = {
                        'source': f'ShiftReg[{register}:{bit}]',
                        'behavior': parse_button_behavior(behavior),
                        'type': 'Shift Register',
                        'reversed': bool(reversed_pin),
                        'latch': parse_latch_mode(latch_mode)
                    }
        
        # Create summary table
        print(f"{'Button':<8} {'Type':<15} {'Source':<15} {'Behavior':<10} {'Options':<15}")
        print("-" * 68)
        
        for button_id in sorted(buttons.keys()):
            details = buttons[button_id]
            options = []
            if details.get('reversed'):
                options.append('Reversed')
            if details.get('latch'):
                options.append(details['latch'])
            
            print(f"{button_id:<8} {details['type']:<15} {details['source']:<15} {details['behavior']:<10} {', '.join(options):<15}")
        
        print(f"\nTotal buttons configured: {len(buttons)}")
        
        # Show encoder pairs
        encoders = [(bid, details) for bid, details in buttons.items() if 'ENC_' in details['behavior']]
        if encoders:
            print(f"\nEncoder pairs:")
            for i in range(0, len(encoders), 2):
                if i + 1 < len(encoders):
                    enc_a = encoders[i]
                    enc_b = encoders[i + 1]
                    print(f"  Encoder {i//2 + 1}: Buttons {enc_a[0]}/{enc_b[0]} on {enc_a[1]['source']}/{enc_b[1]['source']}")
        
    except Exception as e:
        print(f"Error creating button summary: {e}")

def read_config_digital_h():
    """Read and parse the ConfigDigital.h file to show expected configuration"""
    print("\n=== Expected Configuration from ConfigDigital.h ===")
    
    config_file = "src/config/ConfigDigital.h"
    if not os.path.exists(config_file):
        print(f"Config file {config_file} not found")
        return
    
    try:
        with open(config_file, 'r') as f:
            content = f.read()
        
        print("Hardware Pin Map:")
        # Parse hardwarePinMap array
        pin_map_match = re.search(r'static const PinMapEntry hardwarePinMap\[\] = \{(.*?)\};', content, re.DOTALL)
        if pin_map_match:
            pin_entries = pin_map_match.group(1)
            for line in pin_entries.split('\n'):
                line = line.strip()
                if line and not line.startswith('//') and '{' in line:
                    # Extract pin and type from lines like: {"4", BTN},   // Button 9 (MOMENTARY)
                    match = re.search(r'\{"(\d+)",\s*(\w+)\}.*?//\s*(.*)', line)
                    if match:
                        pin, pin_type, comment = match.groups()
                        print(f"  Pin {pin}: {pin_type} - {comment}")
        
        print("\nShift Register Configuration:")
        # Parse SHIFTREG_COUNT
        shiftreg_match = re.search(r'#define\s+SHIFTREG_COUNT\s+(\d+)', content)
        if shiftreg_match:
            shiftreg_count = shiftreg_match.group(1)
            print(f"  Shift register count: {shiftreg_count}")
        
        print("\nLogical Inputs Configuration:")
        # Parse logicalInputs array
        logical_match = re.search(r'constexpr LogicalInput logicalInputs\[\] = \{(.*?)\};', content, re.DOTALL)
        if logical_match:
            logical_entries = logical_match.group(1)
            input_num = 1
            for line in logical_entries.split('\n'):
                line = line.strip()
                if line and not line.startswith('//') and '{' in line:
                    # Parse different input types
                    if 'INPUT_PIN' in line:
                        # Parse: { INPUT_PIN, { .pin = {4, 9, MOMENTARY, 0} } },    // Pin 4 -> Button 9 (MOMENTARY)
                        match = re.search(r'INPUT_PIN.*?\.pin\s*=\s*\{(\d+),\s*(\d+),\s*(\w+),\s*(\d+)\}.*?//\s*(.*)', line)
                        if match:
                            pin, button_id, behavior, reversed_pin, comment = match.groups()
                            print(f"  Input {input_num}: INPUT_PIN - Pin {pin} -> Button {button_id} ({behavior}) - {comment}")
                            input_num += 1
                    elif 'INPUT_MATRIX' in line:
                        # Parse: { INPUT_MATRIX, { .matrix = {0, 0, 3, NORMAL, 0} } },
                        match = re.search(r'INPUT_MATRIX.*?\.matrix\s*=\s*\{(\d+),\s*(\d+),\s*(\d+),\s*(\w+),\s*(\d+)\}.*?//\s*(.*)', line)
                        if match:
                            row, col, button_id, behavior, reversed_pin, comment = match.groups()
                            print(f"  Input {input_num}: INPUT_MATRIX - Row {row}, Col {col} -> Button {button_id} ({behavior}) - {comment}")
                            input_num += 1
                    elif 'INPUT_SHIFTREG' in line:
                        # Parse: { INPUT_SHIFTREG, { .shiftreg = {0, 0, 11, NORMAL, 0} } },     // Reg 0, bit 0 -> Button 11
                        match = re.search(r'INPUT_SHIFTREG.*?\.shiftreg\s*=\s*\{(\d+),\s*(\d+),\s*(\d+),\s*(\w+),\s*(\d+)\}.*?//\s*(.*)', line)
                        if match:
                            reg, bit, button_id, behavior, reversed_pin, comment = match.groups()
                            latch_mode = "FOUR3"  # default
                            # Check if latch mode is specified
                            latch_match = re.search(r'(\w+)\s*\}', line)
                            if latch_match and latch_match.group(1) in ["FOUR0", "FOUR1", "FOUR2", "FOUR3"]:
                                latch_mode = latch_match.group(1)
                            print(f"  Input {input_num}: INPUT_SHIFTREG - Reg {reg}, Bit {bit} -> Button {button_id} ({behavior}, {latch_mode}) - {comment}")
                            input_num += 1
        
    except Exception as e:
        print(f"Error reading ConfigDigital.h: {e}")

def main():
    """Main test function"""
    print("JoyCore Configuration Test Script")
    print("=" * 40)
    
    # Show expected configuration from source
    read_config_digital_h()
    
    # Find device
    device_path = find_joycore_device()
    if not device_path:
        print("Could not find JoyCore device. Make sure the board is connected and firmware is loaded.")
        return 1
    
    # Connect to device
    try:
        device = hid.device()
        device.open_path(device_path)
        
        print(f"\nConnected to device successfully!")
        print(f"Manufacturer: {device.get_manufacturer_string()}")
        print(f"Product: {device.get_product_string()}")
        print(f"Serial: {device.get_serial_number_string()}")
        
        # Run tests
        success = True
        success &= test_get_status(device)
        success &= test_get_config(device)
        
        # Create button summary if config test passed
        if success:
            create_button_summary(device)
        
        if success:
            print("\nAll tests completed successfully!")
        else:
            print("\nSome tests failed!")
        
        device.close()
        return 0 if success else 1
        
    except Exception as e:
        print(f"Error connecting to device: {e}")
        return 1

if __name__ == "__main__":
    try:
        import hid
    except ImportError:
        print("Error: hidapi library not found!")
        print("Install it with: pip install hidapi")
        sys.exit(1)
    
    sys.exit(main())