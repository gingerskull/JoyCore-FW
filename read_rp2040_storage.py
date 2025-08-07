#!/usr/bin/env python3
"""
Read configuration files from RP2040 flash storage (LittleFS)

This script reads the configuration files stored in the RP2040's flash:
- /config.bin - Main configuration file
- /config_backup.bin - Backup configuration
- /fw_version.txt - Firmware version tracking
"""

import serial
import serial.tools.list_ports
import struct
import time
import sys

def find_rp2040_port():
    """Find the RP2040 device COM port"""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for RP2040 device
        if "2E8A" in port.hwid or "RP2040" in port.description:
            return port.device
    return None

def send_command(ser, command):
    """Send a command and read response"""
    ser.write((command + '\n').encode())
    time.sleep(0.1)
    
    response = ""
    while ser.in_waiting:
        response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.05)
    
    return response

def parse_config_header(data):
    """Parse the configuration header structure"""
    if len(data) < 16:
        return None
    
    # ConfigHeader struct:
    # uint32_t magic;          // Magic number (0x4A4F5943 = "JOYC")
    # uint16_t version;        // Configuration format version
    # uint16_t size;           // Total size of configuration data
    # uint32_t checksum;       // CRC32 checksum
    # uint8_t reserved[4];     // Reserved
    
    magic, version, size, checksum = struct.unpack('<IHHII', data[:16])
    
    return {
        'magic': hex(magic),
        'magic_str': ''.join(chr((magic >> (8*i)) & 0xFF) for i in range(4)),
        'version': version,
        'size': size,
        'checksum': hex(checksum)
    }

def parse_usb_descriptor(data, offset):
    """Parse USB descriptor from config data"""
    if len(data) < offset + 76:
        return None, offset
    
    # StoredUSBDescriptor struct:
    # uint16_t vendorID;       // USB Vendor ID
    # uint16_t productID;      // USB Product ID  
    # char manufacturer[32];   // Manufacturer string
    # char product[32];        // Product string
    # uint8_t reserved[8];     // Padding
    
    vid, pid = struct.unpack('<HH', data[offset:offset+4])
    manufacturer = data[offset+4:offset+36].decode('utf-8', errors='ignore').rstrip('\x00')
    product = data[offset+36:offset+68].decode('utf-8', errors='ignore').rstrip('\x00')
    
    return {
        'vendorID': hex(vid),
        'productID': hex(pid),
        'manufacturer': manufacturer,
        'product': product
    }, offset + 76

def parse_stored_config(data):
    """Parse the complete stored configuration"""
    if len(data) < 16:
        print("Error: Config data too small")
        return None
        
    # Parse header
    header = parse_config_header(data)
    if not header:
        print("Error: Failed to parse header")
        return None
    
    print(f"Config Header:")
    print(f"  Magic: {header['magic']} ('{header['magic_str']}')")
    print(f"  Version: {header['version']}")
    print(f"  Size: {header['size']} bytes")
    print(f"  Checksum: {header['checksum']}")
    
    offset = 16  # After header
    
    # Parse USB descriptor
    usb_desc, offset = parse_usb_descriptor(data, offset)
    if usb_desc:
        print(f"\nUSB Descriptor:")
        print(f"  VID: {usb_desc['vendorID']}")
        print(f"  PID: {usb_desc['productID']}")
        print(f"  Manufacturer: '{usb_desc['manufacturer']}'")
        print(f"  Product: '{usb_desc['product']}'")
    
    # Parse counts
    if len(data) >= offset + 4:
        pin_count, input_count, shiftreg_count, _ = struct.unpack('BBBB', data[offset:offset+4])
        print(f"\nConfiguration Counts:")
        print(f"  Pin Map Entries: {pin_count}")
        print(f"  Logical Inputs: {input_count}")
        print(f"  Shift Registers: {shiftreg_count}")
        offset += 4
        
        # Parse axis configs (8 axes * StoredAxisConfig)
        print(f"\nAxis Configuration:")
        for i in range(8):
            if len(data) >= offset + 16:  # Minimal StoredAxisConfig size
                enabled = data[offset]
                if enabled:
                    pin = data[offset + 1]
                    min_val, max_val = struct.unpack('<HH', data[offset+2:offset+6])
                    print(f"  Axis {i}: Enabled, Pin={pin}, Range={min_val}-{max_val}")
                offset += 16  # Advance by StoredAxisConfig size

def read_storage_files():
    """Main function to read storage files from RP2040"""
    
    # Find the RP2040 port
    port = find_rp2040_port()
    if not port:
        print("Error: Could not find RP2040 device")
        print("Available ports:")
        for p in serial.tools.list_ports.comports():
            print(f"  {p.device}: {p.description}")
        return
    
    print(f"Found RP2040 on {port}")
    
    try:
        # Open serial connection
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for connection
        
        # Clear any pending data
        ser.read(ser.in_waiting)
        
        print("\nChecking configuration status...")
        response = send_command(ser, "STATUS")
        print(response)
        
        # Read firmware version file
        print("\n" + "="*60)
        print("Reading /fw_version.txt:")
        print("="*60)
        # Note: This would require implementing file read commands in the firmware
        # For now, we'll parse what we can from STATUS command
        
        # Parse config.bin if we had a way to read it
        # This would require implementing a DUMP_CONFIG command in firmware
        
        print("\nNote: To fully read the storage files, we need to implement")
        print("file dump commands in the firmware (e.g., DUMP_CONFIG command)")
        
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def decode_config_file(filename):
    """Decode a config.bin file from disk (for testing)"""
    try:
        with open(filename, 'rb') as f:
            data = f.read()
        
        print(f"\nDecoding {filename} ({len(data)} bytes)")
        print("="*60)
        parse_stored_config(data)
        
    except FileNotFoundError:
        print(f"File not found: {filename}")
    except Exception as e:
        print(f"Error reading file: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        # Decode a config file from disk
        decode_config_file(sys.argv[1])
    else:
        # Read from RP2040 via serial
        read_storage_files()