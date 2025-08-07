#!/usr/bin/env python3
"""
Read JoyCore configuration via CDC Serial interface

This script communicates with the JoyCore firmware using the CDC serial
interface to read the stored configuration files directly from flash storage.
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

def send_command(ser, command):
    """Send a command and read response"""
    print(f"  [DEBUG] Clearing buffer ({ser.in_waiting} bytes waiting)")
    # Clear any pending data
    ser.read(ser.in_waiting)
    
    print(f"  [DEBUG] Sending command: '{command}'")
    # Send command
    ser.write((command + '\n').encode())
    ser.flush()
    print(f"  [DEBUG] Command sent, waiting for response...")
    
    # Wait for response
    time.sleep(0.1)
    
    response_lines = []
    timeout = time.time() + 5.0  # 5 second timeout
    bytes_received = 0
    
    print(f"  [DEBUG] Starting response loop (timeout in 5s)")
    while time.time() < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            bytes_received += len(line) + 2  # +2 for \r\n
            print(f"  [DEBUG] Received line ({len(line)} chars): '{line}'")
            if line:
                response_lines.append(line)
                # Check for end markers
                if line == "END_FILES" or line.startswith("ERROR:") or line.startswith("FILE_DATA:"):
                    print(f"  [DEBUG] Found end marker, stopping")
                    break
        else:
            time.sleep(0.01)
    
    print(f"  [DEBUG] Response complete: {len(response_lines)} lines, {bytes_received} bytes")
    return response_lines

def parse_file_data(response_line):
    """Parse FILE_DATA response: FILE_DATA:filename:size:hexdata"""
    if not response_line.startswith("FILE_DATA:"):
        return None, None
    
    parts = response_line.split(':', 3)
    if len(parts) != 4:
        return None, None
    
    filename = parts[1]
    size = int(parts[2])
    hex_data = parts[3]
    
    # Convert hex string to bytes
    data = bytes.fromhex(hex_data)
    
    return filename, data

def parse_config_data(data):
    """Parse configuration binary data"""
    if len(data) < 16:
        return None
    
    # Parse header
    magic = struct.unpack('<I', data[0:4])[0]
    version = struct.unpack('<H', data[4:6])[0]
    size = struct.unpack('<H', data[6:8])[0]
    checksum = struct.unpack('<I', data[8:12])[0]
    
    result = {
        'header': {
            'magic': f"0x{magic:08X}",
            'magic_str': ''.join(chr((magic >> (8*i)) & 0xFF) for i in range(4)),
            'version': version,
            'size': size,
            'checksum': f"0x{checksum:08X}"
        }
    }
    
    # Parse USB descriptor if present
    if len(data) >= 92:
        vid = struct.unpack('<H', data[16:18])[0]
        pid = struct.unpack('<H', data[18:20])[0]
        manufacturer = data[20:52].decode('utf-8', errors='ignore').rstrip('\x00')
        product = data[52:84].decode('utf-8', errors='ignore').rstrip('\x00')
        
        result['usb'] = {
            'vendorID': f"0x{vid:04X}",
            'productID': f"0x{pid:04X}",
            'manufacturer': manufacturer,
            'product': product
        }
    
    # Parse configuration counts if present
    if len(data) >= 96:
        result['counts'] = {
            'pinMapCount': data[92],
            'logicalInputCount': data[93],
            'shiftRegCount': data[94]
        }
    
    return result

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
        print("Opening serial connection...")
        ser = serial.Serial(port, 115200, timeout=1)
        print("Serial port opened, waiting for device...")
        time.sleep(2)  # Wait for connection
        
        print("Testing basic communication...")
        # Clear any startup messages
        ser.read(ser.in_waiting)
        
        # Test basic communication first
        print("Sending STATUS command...")
        test_response = send_command(ser, "STATUS")
        print(f"STATUS response: {test_response}")
        
        # Get storage info
        print("\n" + "="*60)
        print("Storage Information:")
        print("="*60)
        
        print("Sending STORAGE_INFO command...")
        response = send_command(ser, "STORAGE_INFO")
        print(f"STORAGE_INFO response: {response}")
        for line in response:
            print(f"  {line}")
        
        # List files
        print("\n" + "="*60)
        print("Available Files:")
        print("="*60)
        
        response = send_command(ser, "LIST_FILES")
        files = []
        for line in response:
            if line == "FILES:":
                continue
            elif line == "END_FILES":
                break
            elif line.startswith("ERROR:"):
                print(f"  {line}")
                break
            else:
                files.append(line)
                print(f"  {line}")
        
        # Read each file
        for filename in files:
            print(f"\n" + "="*60)
            print(f"Reading {filename}:")
            print("="*60)
            
            response = send_command(ser, f"READ_FILE {filename}")
            
            print(f"  Debug - Raw response lines:")
            for i, line in enumerate(response):
                print(f"    [{i}]: '{line}'")
            
            for line in response:
                if line.startswith("FILE_DATA:"):
                    fname, data = parse_file_data(line)
                    if data:
                        print(f"  File: {fname}")
                        print(f"  Size: {len(data)} bytes")
                        
                        if filename == "/config.bin":
                            # Parse config data
                            config = parse_config_data(data)
                            if config:
                                print(f"  Magic: {config['header']['magic']} ('{config['header']['magic_str']}')")
                                print(f"  Version: {config['header']['version']}")
                                print(f"  Checksum: {config['header']['checksum']}")
                                
                                if 'usb' in config:
                                    print(f"  USB VID: {config['usb']['vendorID']}")
                                    print(f"  USB PID: {config['usb']['productID']}")
                                    print(f"  Manufacturer: '{config['usb']['manufacturer']}'")
                                    print(f"  Product: '{config['usb']['product']}'")
                                
                                if 'counts' in config:
                                    print(f"  Pin Map Entries: {config['counts']['pinMapCount']}")
                                    print(f"  Logical Inputs: {config['counts']['logicalInputCount']}")
                                    print(f"  Shift Registers: {config['counts']['shiftRegCount']}")
                        
                        elif filename == "/fw_version.txt":
                            # Parse firmware version
                            version_str = data.decode('utf-8', errors='ignore').strip()
                            print(f"  Firmware Version: {version_str}")
                        
                        else:
                            # Show hex dump for other files
                            print("  Hex dump:")
                            for i in range(0, min(len(data), 128), 16):
                                hex_part = ' '.join(f'{data[j]:02X}' for j in range(i, min(i+16, len(data))))
                                ascii_part = ''.join(chr(data[j]) if 32 <= data[j] <= 126 else '.' for j in range(i, min(i+16, len(data))))
                                print(f"    {i:04X}: {hex_part:<48} {ascii_part}")
                
                elif line.startswith("ERROR:"):
                    print(f"  {line}")
                    break
        
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