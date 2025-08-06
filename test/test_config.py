#!/usr/bin/env python3
"""
JoyCore Configuration Test Script
Tests the USB HID configuration protocol with the board's storage-based configuration system
"""

import hid
import struct
import time
import sys

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
        if device['vendor_id'] == 0x2E8A and device['product_id'] == 0xA02F:
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
    """Parse configuration data using the new StoredConfig structure"""
    print("\n=== Parsing Configuration Data ===")
    
    if not config_data or len(config_data) < 20:  # ConfigHeader is 20 bytes
        print("Insufficient configuration data")
        return
        
    try:
        # StoredConfig structure (based on src/config/core/ConfigStructs.h):
        # struct ConfigHeader {
        #     uint32_t magic;          // Magic number for validation (0x4A4F5943 = "JOYC")
        #     uint16_t version;        // Configuration format version
        #     uint16_t size;           // Total size of configuration data
        #     uint32_t checksum;       // CRC32 checksum for data integrity
        #     uint8_t reserved[4];     // Reserved for future use
        # }
        
        offset = 0
        
        # Parse config header
        magic = struct.unpack('<I', config_data[offset:offset+4])[0]
        offset += 4
        
        version = struct.unpack('<H', config_data[offset:offset+2])[0]
        offset += 2
        
        size = struct.unpack('<H', config_data[offset:offset+2])[0]
        offset += 2
        
        checksum = struct.unpack('<I', config_data[offset:offset+4])[0]
        offset += 4
        
        # Skip reserved bytes
        offset += 4
        
        print(f"Config Magic: 0x{magic:08X} ({'JOYC' if magic == 0x4A4F5943 else 'INVALID'})")
        print(f"Config Version: {version}")
        print(f"Config Size: {size} bytes")
        print(f"Config Checksum: 0x{checksum:08X}")
        
        if len(config_data) < size:
            print(f"Warning: Received {len(config_data)} bytes but config size is {size}")
        
        # Parse main structure data
        if offset + 4 <= len(config_data):
            pin_map_count = config_data[offset]
            offset += 1
            
            logical_input_count = config_data[offset]
            offset += 1
            
            shift_reg_count = config_data[offset]
            offset += 1
            
            # Skip reserved byte
            offset += 1
            
            print(f"\nConfiguration Summary:")
            print(f"  Pin map entries: {pin_map_count}")
            print(f"  Logical inputs: {logical_input_count}")
            print(f"  Shift registers: {shift_reg_count}")
            
            # Parse 8 analog axes (each is 16 bytes)
            print(f"\n=== Analog Axes Configuration ===")
            for axis_id in range(8):
                if offset + 16 > len(config_data):
                    break
                    
                enabled = config_data[offset]
                pin = config_data[offset + 1]
                min_value = struct.unpack('<H', config_data[offset + 2:offset + 4])[0]
                max_value = struct.unpack('<H', config_data[offset + 4:offset + 6])[0]
                filter_level = config_data[offset + 6]
                ewma_alpha = struct.unpack('<H', config_data[offset + 7:offset + 9])[0]
                deadband = struct.unpack('<H', config_data[offset + 9:offset + 11])[0]
                curve = config_data[offset + 11]
                
                axis_names = ["X", "Y", "Z", "RX", "RY", "RZ", "S1", "S2"]
                if enabled:
                    print(f"  Axis {axis_names[axis_id]}: Pin {pin}, Range {min_value}-{max_value}, Filter {filter_level}, Curve {curve}")
                
                offset += 16
                
            # Parse variable-length pin map entries
            print(f"\n=== Hardware Pin Map ===")
            for i in range(pin_map_count):
                if offset + 10 > len(config_data):
                    break
                    
                # StoredPinMapEntry: name[8], type, reserved
                name = config_data[offset:offset+8].decode('ascii', errors='ignore').rstrip('\x00')
                pin_type = config_data[offset + 8]
                
                print(f"  Pin {name}: Type {pin_type}")
                offset += 10
            
            # Parse logical inputs
            print(f"\n=== Button Configuration ===")
            for i in range(logical_input_count):
                if offset + 16 > len(config_data):
                    break
                    
                # StoredLogicalInput structure (16 bytes)
                input_type = config_data[offset]
                behavior = config_data[offset + 1]
                joy_button_id = config_data[offset + 2]
                reverse = config_data[offset + 3]
                encoder_latch_mode = config_data[offset + 4]
                # Skip reserved bytes
                
                print(f"  Input {i+1}:")
                print(f"    Type: {parse_input_type(input_type)}")
                print(f"    Button ID: {joy_button_id}")
                print(f"    Behavior: {parse_button_behavior(behavior)}")
                print(f"    Reversed: {bool(reverse)}")
                print(f"    Latch Mode: {parse_latch_mode(encoder_latch_mode)}")
                
                # Parse the union data (starts at offset + 8)
                if input_type == 0:  # INPUT_PIN
                    pin = config_data[offset + 8]
                    print(f"    Pin: {pin}")
                elif input_type == 1:  # INPUT_MATRIX
                    row = config_data[offset + 8]
                    col = config_data[offset + 9]
                    print(f"    Matrix Position: Row {row}, Col {col}")
                elif input_type == 2:  # INPUT_SHIFTREG
                    reg_index = config_data[offset + 8]
                    bit_index = config_data[offset + 9]
                    print(f"    Shift Register: {reg_index}, Bit: {bit_index}")
                
                print()
                offset += 16
                
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
    """Create a summary table of all configured buttons from board storage"""
    print("\n=== Button Configuration Summary ===")
    
    try:
        # Get configuration from device storage
        message = create_config_message(ConfigMessageType.GET_CONFIG)
        device.send_feature_report(message)
        time.sleep(0.1)
        response = device.get_feature_report(CONFIG_USB_FEATURE_REPORT_ID, CONFIG_USB_MAX_PACKET_SIZE)
        parsed = parse_config_response(response)
        
        if not parsed or parsed['data_length'] <= 0:
            print("Could not retrieve configuration from device storage")
            return
        
        # Parse the config to extract button mappings using new StoredConfig format
        config_data = parsed['data']
        buttons = {}  # button_id -> details
        
        # Parse header and main structure
        if len(config_data) < 20:
            print("Invalid configuration data received")
            return
            
        offset = 20  # Skip ConfigHeader (20 bytes)
        
        # Parse main structure counts
        if offset + 4 <= len(config_data):
            pin_map_count = config_data[offset]
            logical_input_count = config_data[offset + 1] 
            shift_reg_count = config_data[offset + 2]
            offset += 4  # Skip reserved byte
            
            # Skip analog axes (8 * 16 bytes = 128 bytes)
            offset += 128
            
            # Skip pin map entries (pin_map_count * 10 bytes each)
            offset += pin_map_count * 10
            
            # Parse logical inputs (16 bytes each)
            for i in range(logical_input_count):
                if offset + 16 > len(config_data):
                    break
                    
                input_type = config_data[offset]
                behavior = config_data[offset + 1]
                joy_button_id = config_data[offset + 2]
                reverse = config_data[offset + 3]
                encoder_latch_mode = config_data[offset + 4]
                
                # Parse union data (starts at offset + 8)
                source = ""
                input_type_name = ""
                
                if input_type == 0:  # INPUT_PIN
                    pin = config_data[offset + 8]
                    source = f'Pin {pin}'
                    input_type_name = 'Direct Pin'
                elif input_type == 1:  # INPUT_MATRIX
                    row = config_data[offset + 8]
                    col = config_data[offset + 9]
                    source = f'Matrix[{row},{col}]'
                    input_type_name = 'Button Matrix'
                elif input_type == 2:  # INPUT_SHIFTREG
                    reg_index = config_data[offset + 8]
                    bit_index = config_data[offset + 9]
                    source = f'ShiftReg[{reg_index}:{bit_index}]'
                    input_type_name = 'Shift Register'
                
                buttons[joy_button_id] = {
                    'source': source,
                    'behavior': parse_button_behavior(behavior),
                    'type': input_type_name,
                    'reversed': bool(reverse),
                    'latch': parse_latch_mode(encoder_latch_mode)
                }
                
                offset += 16
        
        # Create summary table
        print(f"{'Button':<8} {'Type':<15} {'Source':<15} {'Behavior':<10} {'Options':<15}")
        print("-" * 68)
        
        for button_id in sorted(buttons.keys()):
            details = buttons[button_id]
            options = []
            if details.get('reversed'):
                options.append('Reversed')
            if details.get('latch') and details['latch'] != 'UNKNOWN(0)':
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

def main():
    """Main test function"""
    print("JoyCore Configuration Test Script")
    print("=" * 40)
    print("Reading configuration from board storage only")
    
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