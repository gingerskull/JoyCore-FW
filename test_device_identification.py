#!/usr/bin/env python3
"""
JoyCore-FW Device Identification Test Script

This script tests the device identification protocol by:
1. Scanning all serial ports
2. Sending the IDENTIFY command
3. Verifying the response format
4. Displaying found devices

Usage: python test_device_identification.py
"""

import serial
import serial.tools.list_ports
import time
import sys

# Expected fixed parts of the response
EXPECTED_PREFIX = "JOYCORE_ID"
EXPECTED_SIGNATURE = "JOYCORE-FW"
EXPECTED_MAGIC = "4A4F5943"

def test_joycore_identification():
    """Test the JoyCore-FW device identification protocol."""
    print("=" * 60)
    print("JoyCore-FW Device Identification Test")
    print("=" * 60)
    print()
    
    # Get all available serial ports
    ports = list(serial.tools.list_ports.comports())
    
    if not ports:
        print("❌ No serial ports found on this system")
        return False
    
    print(f"Found {len(ports)} serial port(s) to scan:")
    for port in ports:
        print(f"  - {port.device}: {port.description}")
    print()
    
    joycore_devices = []
    tested_ports = 0
    
    for port in ports:
        tested_ports += 1
        print(f"Testing port {tested_ports}/{len(ports)}: {port.device}...", end=" ")
        
        try:
            # Open serial connection
            ser = serial.Serial(
                port=port.device,
                baudrate=115200,
                timeout=1,
                write_timeout=1
            )
            
            # Small delay for port stability
            time.sleep(0.1)
            
            # Clear any pending data
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # Send IDENTIFY command
            ser.write(b"IDENTIFY\n")
            
            # Read response
            response = ser.readline().decode('utf-8', errors='ignore').strip()
            
            if response:
                # Parse response
                parts = response.split(':')
                
                # Check if this is a valid JoyCore response
                if (len(parts) >= 4 and 
                    parts[0] == EXPECTED_PREFIX and
                    parts[1] == EXPECTED_SIGNATURE and
                    parts[2] == EXPECTED_MAGIC):
                    
                    firmware_version = parts[3]
                    print(f"✅ JoyCore-FW v{firmware_version}")
                    joycore_devices.append({
                        'port': port.device,
                        'description': port.description,
                        'firmware_version': firmware_version,
                        'full_response': response
                    })
                else:
                    print(f"❌ Not JoyCore (got: {response[:50]})")
            else:
                print("❌ No response")
            
            ser.close()
            
        except serial.SerialException as e:
            print(f"❌ Port error: {str(e)[:30]}")
        except Exception as e:
            print(f"❌ Unexpected error: {str(e)[:30]}")
    
    print()
    print("=" * 60)
    print("Test Results")
    print("=" * 60)
    
    if joycore_devices:
        print(f"✅ Found {len(joycore_devices)} JoyCore-FW device(s):\n")
        for idx, device in enumerate(joycore_devices, 1):
            print(f"Device {idx}:")
            print(f"  Port:        {device['port']}")
            print(f"  Description: {device['description']}")
            print(f"  Firmware:    v{device['firmware_version']}")
            print(f"  Response:    {device['full_response']}")
            print()
        
        # Verify protocol format
        print("Protocol Verification:")
        for check, status in [
            ("Fixed prefix 'JOYCORE_ID'", all(d['full_response'].startswith(EXPECTED_PREFIX) for d in joycore_devices)),
            ("Fixed signature 'JOYCORE-FW'", all(EXPECTED_SIGNATURE in d['full_response'] for d in joycore_devices)),
            ("Fixed magic '4A4F5943'", all(EXPECTED_MAGIC in d['full_response'] for d in joycore_devices)),
        ]:
            print(f"  {check}: {'✅ PASS' if status else '❌ FAIL'}")
        
        return True
    else:
        print("❌ No JoyCore-FW devices found")
        print()
        print("Troubleshooting tips:")
        print("  1. Ensure the JoyCore-FW device is connected via USB")
        print("  2. Check that the device appears as a COM/serial port")
        print("  3. Verify the firmware includes the IDENTIFY command")
        print("  4. Try closing other programs using the serial port")
        return False

def main():
    """Main entry point."""
    try:
        success = test_joycore_identification()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\nTest cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Test failed with error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()