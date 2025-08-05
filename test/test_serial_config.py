#!/usr/bin/env python3
"""
JoyCore Configuration Test Script - Serial Interface
Tests the configuration system via serial commands instead of USB HID
"""

import serial
import time
import sys
import json

def find_joycore_serial():
    """Find the JoyCore device serial port"""
    import serial.tools.list_ports
    
    print("Scanning for serial ports...")
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        print(f"  {port.device}: {port.description}")
        # Look for RP2040 Pico, Arduino, or USB Serial devices
        desc = str(port.description).lower()
        if "pico" in desc or "arduino" in desc or "usb serial" in desc or "rp2040" in desc:
            print(f"  -> Potential JoyCore device found: {port.device}")
            return port.device
    
    return None

def test_serial_commands(ser):
    """Test basic serial communication"""
    print("\n=== Testing Serial Communication ===")
    
    try:
        # Send a simple test command
        ser.write(b'STATUS\n')
        time.sleep(0.1)
        
        # Read response
        if ser.in_waiting > 0:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"Response: {response}")
            return True
        else:
            print("No response received")
            return False
            
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    """Main test function"""
    print("JoyCore Configuration Test Script - Serial Interface")
    print("=" * 50)
    
    # Find serial port
    port = find_joycore_serial()
    if not port:
        print("Could not find JoyCore serial device.")
        print("Make sure the board is connected and enumerated as a serial device.")
        return 1
    
    # Connect to serial port
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for connection to stabilize
        
        print(f"Connected to {port} successfully!")
        
        # Test basic communication
        success = test_serial_commands(ser)
        
        if success:
            print("\nSerial communication test completed successfully!")
        else:
            print("\nSerial communication test failed!")
        
        ser.close()
        return 0 if success else 1
        
    except Exception as e:
        print(f"Error connecting to serial port: {e}")
        return 1

if __name__ == "__main__":
    try:
        import serial
    except ImportError:
        print("Error: pyserial library not found!")
        print("Install it with: pip install pyserial")
        sys.exit(1)
    
    sys.exit(main())