#!/usr/bin/env python3
"""
Test script to verify storage system fixes for JoyCore
Tests that file listing now queries actual storage and files can be read
"""

import serial
import serial.tools.list_ports
import time
import sys
import struct

def find_joycore_serial_port():
    """Find the JoyCore CDC serial port"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for JoyCore device - check VID/PID or description
        if ("2E8A" in port.hwid and "A02F" in port.hwid) or "Joycore" in port.description:
            return port.device
    return None

def send_command(ser, command, wait_time=0.1):
    """Send a command and read response"""
    # Clear any pending data
    ser.read(ser.in_waiting)
    
    # Send command
    ser.write((command + '\n').encode())
    ser.flush()
    
    # Wait for response
    time.sleep(wait_time)
    
    response_lines = []
    timeout = time.time() + 5.0  # 5 second timeout
    
    while time.time() < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                response_lines.append(line)
                # Check for end markers
                if line == "END_FILES" or line.startswith("ERROR:") or line.startswith("FILE_DATA:"):
                    break
        else:
            time.sleep(0.01)
    
    return response_lines

def hex_dump(data, offset=0, length=None):
    """Create a hex dump of binary data"""
    if isinstance(data, str):
        data = bytes.fromhex(data)
    
    if length:
        data = data[:length]
    
    lines = []
    for i in range(0, len(data), 16):
        hex_part = ' '.join(f'{data[j]:02X}' if j < len(data) else '  ' for j in range(i, i+16))
        ascii_part = ''.join(chr(data[j]) if 32 <= data[j] <= 126 else '.' for j in range(i, min(i+16, len(data))))
        lines.append(f"{offset+i:04X}: {hex_part:<48} {ascii_part}")
    return '\n'.join(lines)

def decode_config_data(hex_data):
    """Decode configuration binary data into human-readable format"""
    try:
        # Convert hex string to bytes
        data = bytes.fromhex(hex_data)
        
        # First, show a hex dump of key sections
        print("\n=== HEX DUMP OF KEY SECTIONS ===")
        print("Header (0x00-0x0F):")
        print(hex_dump(data[0:16], 0))
        print("\nUSB Descriptor (0x10-0x5B):")
        print(hex_dump(data[16:92], 16))
        print("\nCounts (0x5C-0x5F):")
        print(hex_dump(data[92:96], 92))
        print("\nAxis 0 (0x60-0x6F):")
        print(hex_dump(data[96:112], 96))
        print("\nAxis 1 (0x70-0x7F):")
        print(hex_dump(data[112:128], 112))
        print("=== END HEX DUMP ===\n")
        
        if len(data) < 16:
            return {"error": "Data too short for valid config"}
        
        result = {
            "header": {},
            "counts": {},
            "axes": [],
            "usb": {},
            "pinMap": [],
            "logicalInputs": []
        }
        
        # Parse header (16 bytes total: magic(4) + version(2) + size(2) + checksum(4) + reserved(4))
        magic = struct.unpack('<I', data[0:4])[0]
        version = struct.unpack('<H', data[4:6])[0]
        size = struct.unpack('<H', data[6:8])[0]
        checksum = struct.unpack('<I', data[8:12])[0]
        # reserved[4] at bytes 12-15
        
        result['header'] = {
            'magic': f"0x{magic:08X}",
            'magic_str': ''.join(chr((magic >> (8*i)) & 0xFF) for i in range(4)),
            'version': version,
            'size': size,
            'checksum': f"0x{checksum:08X}"
        }
        
        # Check magic number (0x4A4F5943 = "JOYC")
        if magic != 0x4A4F5943:  # "JOYC" in little-endian
            result['header']['valid'] = False
            result['header']['error'] = f"Invalid magic: expected 0x4A4F5943 ('JOYC'), got 0x{magic:08X}"
        else:
            result['header']['valid'] = True
        
        if len(data) < 224:  # Minimum size for StoredConfig with axes
            return result  # Not enough data for full config
        
        # Parse USB descriptor (76 bytes at offset 16)
        # Structure: vendorID(2) + productID(2) + manufacturer(32) + product(32) + reserved(8)
        if len(data) >= 92:
            usb_offset = 16
            result['usb'] = {
                'vendorID': f"0x{struct.unpack('<H', data[usb_offset:usb_offset + 2])[0]:04X}",
                'productID': f"0x{struct.unpack('<H', data[usb_offset + 2:usb_offset + 4])[0]:04X}",
                'manufacturer': data[usb_offset + 4:usb_offset + 36].decode('utf-8', errors='ignore').rstrip('\x00'),
                'product': data[usb_offset + 36:usb_offset + 68].decode('utf-8', errors='ignore').rstrip('\x00')
            }
        
        # Parse counts (4 bytes at offset 92)
        result['counts'] = {
            'pinMapCount': data[92] if len(data) > 92 else 0,
            'logicalInputCount': data[93] if len(data) > 93 else 0,
            'shiftRegCount': data[94] if len(data) > 94 else 0
        }
        
        # Parse axes configuration (8 axes * 15 bytes each = 120 bytes, starting at offset 96)  
        # StoredAxisConfig structure (packed, exactly 15 bytes):
        # enabled(1) + pin(1) + minValue(2) + maxValue(2) + filterLevel(1) + ewmaAlpha(2) +
        # deadband(2) + curve(1) + reserved[3](3) = 15 bytes
        result['allAxes'] = []  # Store all axes for debugging
        for i in range(8):
            if len(data) >= 96 + (i+1)*15:
                axis_offset = 96 + i*15  # 15 bytes per axis in storage
                
                # Debug: show raw bytes for this axis (exactly 15 bytes)
                raw_bytes = data[axis_offset:axis_offset + 15]  # Structure is exactly 15 bytes
                
                axis = {
                    'index': i,
                    'enabled': data[axis_offset],
                    'pin': data[axis_offset + 1],
                    'minValue': struct.unpack('<H', data[axis_offset + 2:axis_offset + 4])[0],
                    'maxValue': struct.unpack('<H', data[axis_offset + 4:axis_offset + 6])[0],
                    'filterLevel': data[axis_offset + 6],
                    'ewmaAlpha': struct.unpack('<H', data[axis_offset + 7:axis_offset + 9])[0],
                    'deadband': struct.unpack('<H', data[axis_offset + 9:axis_offset + 11])[0],
                    'curve': data[axis_offset + 11],
                    'raw_hex': raw_bytes.hex()
                    # reserved[3] at axis_offset + 12-14
                }
                
                # Decode filter level
                filter_names = {0: "OFF", 1: "LOW", 2: "MEDIUM", 3: "HIGH", 4: "ADAPTIVE"}
                axis['filterName'] = filter_names.get(axis['filterLevel'], f"UNKNOWN({axis['filterLevel']})")
                
                # Decode curve type
                curve_names = {0: "LINEAR", 1: "EXPONENTIAL", 2: "LOGARITHMIC", 3: "S_CURVE"}
                axis['curveName'] = curve_names.get(axis['curve'], f"UNKNOWN({axis['curve']})")
                
                # Decode Arduino pin constants
                pin_names = {27: "A1", 28: "A2", 29: "A3", 26: "A0"}
                axis['pinName'] = pin_names.get(axis['pin'], str(axis['pin']))
                
                result['allAxes'].append(axis)
                
                # Only include axes with enabled == 1 (not 0xFF or other values)
                # EEPROM uninitialized memory is typically 0xFF
                if axis['enabled'] == 1:
                    result['axes'].append(axis)
                elif axis['enabled'] != 0 and axis['enabled'] != 0xFF:
                    # Flag suspicious enabled values
                    axis['suspicious'] = True
                    result['axes'].append(axis)
        
        # Parse variable data (pin map and logical inputs)
        # Variable data starts after the fixed StoredConfig structure
        # Header(16) + USB(76) + counts(4) + axes(8*15=120) = 216 bytes
        variable_offset = 216
        
        # Parse pin map entries (10 bytes each: name[8] + type + reserved)
        pin_map_count = result['counts']['pinMapCount']
        print(f"\nDEBUG: Parsing {pin_map_count} pin map entries at offset {variable_offset}")
        print(f"  Total file size: {len(data)} bytes")
        pin_map_size = pin_map_count * 10
        logical_input_size = result['counts']['logicalInputCount'] * 10  # StoredLogicalInput is 10 bytes, not 12
        expected_total = variable_offset + pin_map_size + logical_input_size
        print(f"  Pin map size: {pin_map_size} bytes, Logical input size: {logical_input_size} bytes")
        print(f"  Expected total size: {expected_total} bytes")
        print(f"  Size difference: {len(data)} - {expected_total} = {len(data) - expected_total} bytes")
        for i in range(pin_map_count):
            if len(data) >= variable_offset + 10:
                entry_data = data[variable_offset:variable_offset + 10]
                print(f"  PinMap[{i}] raw: {entry_data.hex()}")
                
                # Try to decode the name
                name_bytes = entry_data[0:8]
                name = name_bytes.decode('utf-8', errors='ignore').rstrip('\x00')
                
                pin_entry = {
                    'index': i,
                    'name': name,
                    'nameHex': name_bytes.hex(),
                    'type': entry_data[8]
                }
                # Decode pin type
                pin_types = {0: "UNUSED", 1: "BTN", 2: "BTN_GND", 3: "BTN_ROW", 4: "BTN_COL",
                            5: "ENC_A", 6: "ENC_B", 7: "ANALOG"}
                pin_entry['typeName'] = pin_types.get(pin_entry['type'], f"UNKNOWN({pin_entry['type']})")
                result['pinMap'].append(pin_entry)
                variable_offset += 10
        
        # Parse logical inputs (10 bytes each)  
        # Structure: type(1) + behavior(1) + joyButtonID(1) + reverse(1) + encoderLatchMode(1) + reserved[3](3) + union data(2)
        logical_count = result['counts']['logicalInputCount']
        print(f"\nDEBUG: Parsing {logical_count} logical inputs at offset {variable_offset}")
        for i in range(logical_count):
            if len(data) >= variable_offset + 10:
                input_data = data[variable_offset:variable_offset + 10]
                print(f"  LogicalInput[{i}] raw: {input_data.hex()}")
                
                input_type = input_data[0]
                
                logical_input = {
                    'index': i,
                    'type': input_type,
                    'behavior': input_data[1],
                    'joyButtonID': input_data[2],
                    'reverse': input_data[3],
                    'encoderLatchMode': input_data[4],
                    'rawHex': input_data.hex()
                }
                
                # Decode input type (based on Config.h enum InputType)
                input_types = {0: "INPUT_PIN", 1: "INPUT_MATRIX", 2: "INPUT_SHIFTREG"}
                logical_input['typeName'] = input_types.get(input_type, f"UNKNOWN({input_type})")
                
                # Union data starts at offset 8 in the structure (only 2 bytes)
                union_data = input_data[8:]
                
                # Decode based on type
                if input_type == 0:  # INPUT_PIN
                    logical_input['pin'] = union_data[0]
                elif input_type == 1:  # INPUT_MATRIX
                    logical_input['row'] = union_data[0]
                    logical_input['col'] = union_data[1]
                elif input_type == 2:  # INPUT_SHIFTREG
                    logical_input['regIndex'] = union_data[0]
                    logical_input['bitIndex'] = union_data[1]
                elif input_type == 4:  # INPUT_ENCODER
                    # For encoder, the structure might be different - need to check actual implementation
                    logical_input['encoderData'] = union_data[:2].hex()
                
                result['logicalInputs'].append(logical_input)
                variable_offset += 10
            else:
                print(f"  Not enough data for LogicalInput[{i}]: need {variable_offset + 10}, have {len(data)}")
        
        result['totalSize'] = len(data)
        result['parsedSize'] = variable_offset
        
        return result
        
    except Exception as e:
        return {"error": f"Failed to decode config: {str(e)}"}

def print_config(config):
    """Print decoded configuration in a readable format"""
    print("\n=== CONFIGURATION DECODE ===")
    
    # Header
    print("\nHEADER:")
    if 'header' in config:
        h = config['header']
        print(f"  Magic: {h.get('magic', 'N/A')} ('{h.get('magic_str', '')}') - {h.get('valid', False) and 'VALID' or 'INVALID'}")
        print(f"  Version: {h.get('version', 'N/A')}")
        print(f"  Size: {h.get('size', 'N/A')} bytes")
        print(f"  Checksum: {h.get('checksum', 'N/A')}")
        if 'error' in h:
            print(f"  ERROR: {h['error']}")
    
    # USB Descriptor
    print("\nUSB DESCRIPTOR:")
    if 'usb' in config:
        u = config['usb']
        print(f"  VID: {u.get('vendorID', 'N/A')}")
        print(f"  PID: {u.get('productID', 'N/A')}")
        print(f"  Manufacturer: '{u.get('manufacturer', 'N/A')}'")
        print(f"  Product: '{u.get('product', 'N/A')}'")
    
    # Counts
    print("\nCOUNTS:")
    if 'counts' in config:
        c = config['counts']
        print(f"  Pin Map Entries: {c.get('pinMapCount', 0)}")
        print(f"  Logical Inputs: {c.get('logicalInputCount', 0)}")
        print(f"  Shift Registers: {c.get('shiftRegCount', 0)}")
    
    # Axes
    if 'axes' in config and config['axes']:
        print(f"\nAXES ({len(config['axes'])} enabled):")
        for axis in config['axes']:
            print(f"  Axis {axis['index']}:")
            print(f"    Pin: {axis['pinName']} ({axis['pin']})")
            print(f"    Range: {axis['minValue']} - {axis['maxValue']}")
            print(f"    Filter: {axis['filterName']} (ewmaAlpha={axis['ewmaAlpha']})")
            print(f"    Deadband: {axis['deadband']}")
            print(f"    Curve: {axis['curveName']}")
    
    # Debug: Show all axes raw data
    if 'allAxes' in config:
        print("\nDEBUG - ALL AXES RAW DATA:")
        for axis in config['allAxes']:
            if axis['enabled'] == 1:
                status = "ENABLED"
            elif axis['enabled'] == 0:
                status = "disabled"
            elif axis['enabled'] == 0xFF:
                status = "UNINIT(0xFF)"
            else:
                status = f"INVALID({axis['enabled']:02X})"
            
            print(f"  Axis {axis['index']} [{status}]: enabled={axis['enabled']:02X} pin={axis['pin']:02X}")
            print(f"       Raw: {axis['raw_hex']}")
    
    # Pin Map
    if 'pinMap' in config and config['pinMap']:
        print(f"\nPIN MAP ({len(config['pinMap'])} entries):")
        for pin in config['pinMap']:
            print(f"  [{pin['index']}] Pin '{pin['name']}': {pin['typeName']} (name hex: {pin['nameHex']})")
    
    # Logical Inputs
    if 'logicalInputs' in config and config['logicalInputs']:
        print(f"\nLOGICAL INPUTS ({len(config['logicalInputs'])} entries):")
        for inp in config['logicalInputs']:
            print(f"  [{inp['index']}] {inp['typeName']}:")
            print(f"    Button ID: {inp['joyButtonID']}, Behavior: {inp.get('behavior', 'N/A')}, Reverse: {inp['reverse']}")
            if inp['type'] == 0:  # INPUT_PIN
                print(f"    Pin: {inp.get('pin', 'N/A')}")
            elif inp['type'] == 1:  # INPUT_MATRIX
                print(f"    Matrix[{inp.get('row', 'N/A')},{inp.get('col', 'N/A')}]")
            elif inp['type'] == 2:  # INPUT_SHIFTREG
                print(f"    ShiftReg[{inp.get('regIndex', 'N/A')}].bit{inp.get('bitIndex', 'N/A')}")
            elif inp['type'] == 4:  # INPUT_ENCODER
                print(f"    Encoder data: {inp.get('encoderData', 'N/A')}")
    
    if 'totalSize' in config:
        print(f"\nTOTAL SIZE: {config['totalSize']} bytes (parsed: {config['parsedSize']} bytes)")
    
    if 'error' in config:
        print(f"\nERROR: {config['error']}")
    
    print("\n=== END CONFIGURATION ===")

def main():
    # Find the serial port
    port = find_joycore_serial_port()
    if not port:
        print("Error: Could not find JoyCore serial device")
        print("Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"  {p.device}: {p.description} ({p.hwid})")
        return
    
    print(f"Found JoyCore on {port}")
    
    try:
        # Open serial connection
        print("\n1. Opening serial connection...")
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for connection
        
        # Clear startup messages and get initial debug output
        initial_data = ser.read(ser.in_waiting)
        if initial_data:
            print("\n=== STARTUP DEBUG OUTPUT ===")
            print(initial_data.decode('utf-8', errors='ignore'))
            print("=== END STARTUP DEBUG ===\n")
        
        # Test basic communication
        print("2. Testing basic communication...")
        response = send_command(ser, "STATUS")
        print(f"   STATUS response: {response[0] if response else 'No response'}")
        
        # Debug storage state
        print("\n3. Debugging storage state...")
        response = send_command(ser, "DEBUG_STORAGE", wait_time=0.5)
        print("   Storage debug output:")
        for line in response:
            print(f"     {line}")
        
        # Get storage info
        print("\n4. Getting storage information...")
        response = send_command(ser, "STORAGE_INFO")
        for line in response:
            print(f"   {line}")
        
        # List files (now should query actual storage)
        print("\n5. Listing files (from actual storage)...")
        response = send_command(ser, "LIST_FILES")
        files = []
        in_file_list = False
        for line in response:
            if line == "FILES:":
                in_file_list = True
                print("   Found files:")
            elif line == "END_FILES":
                break
            elif in_file_list:
                files.append(line)
                print(f"     - {line}")
        
        if not files:
            print("   No files found in storage!")
            
            # Try to create test files
            print("\n6. Creating test files...")
            response = send_command(ser, "CREATE_TEST_FILES", wait_time=1.0)
            for line in response:
                print(f"   {line}")
            
            # List files again
            print("\n7. Listing files after creation...")
            response = send_command(ser, "LIST_FILES")
            files = []
            in_file_list = False
            for line in response:
                if line == "FILES:":
                    in_file_list = True
                    print("   Found files:")
                elif line == "END_FILES":
                    break
                elif in_file_list:
                    files.append(line)
                    print(f"     - {line}")
        
        # Try to read each file
        if files:
            print("\n8. Reading files...")
            for filename in files:
                print(f"\n   Reading {filename}:")
                response = send_command(ser, f"READ_FILE {filename}")
                
                for line in response:
                    if line.startswith("FILE_DATA:"):
                        parts = line.split(':', 3)
                        if len(parts) >= 3:
                            fname = parts[1]
                            size = parts[2]
                            print(f"     File: {fname}")
                            print(f"     Size: {size} bytes")
                            if len(parts) > 3:
                                hex_data = parts[3]
                                if filename == "/fw_version.txt":
                                    # Decode firmware version
                                    try:
                                        data = bytes.fromhex(hex_data)
                                        version = data.decode('utf-8', errors='ignore')
                                        print(f"     Content: '{version}'")
                                    except:
                                        print(f"     Raw hex: {hex_data[:64]}...")
                                elif filename == "/config.bin":
                                    # Decode and display configuration
                                    config = decode_config_data(hex_data)
                                    print_config(config)
                                else:
                                    print(f"     Data (hex): {hex_data[:64]}...")
                    elif line.startswith("ERROR:"):
                        print(f"     {line}")
                        break
        
        print("\n9. Test complete!")
        print("\nSUMMARY:")
        print(f"  - Storage initialized: {'YES' if 'STORAGE_INITIALIZED:YES' in str(response) else 'UNKNOWN'}")
        print(f"  - Files found: {len(files)}")
        print(f"  - Files readable: {'YES' if files and not any('ERROR:' in str(r) for r in response) else 'NO'}")
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()