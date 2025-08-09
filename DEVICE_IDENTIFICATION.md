# JoyCore-FW Device Identification Protocol

## Overview
JoyCore-FW implements a simple, reliable serial-based identification protocol that allows configuration programs to detect JoyCore-FW devices regardless of user-configured USB VID/PID values.

## Fixed Identifiers
The following identifiers are **FIXED** and will **NEVER CHANGE** in any JoyCore-FW version:

- **Device Signature**: `JOYCORE-FW`
- **Magic Number**: `0x4A4F5943` (hex representation of "JOYC")
- **Response Prefix**: `JOYCORE_ID`

## Serial Protocol

### Discovery Command
```
IDENTIFY\n
```

### Response Format
```
JOYCORE_ID:JOYCORE-FW:4A4F5943:<FIRMWARE_VERSION>\n
```

#### Response Fields:
1. `JOYCORE_ID` - Fixed prefix for easy parsing
2. `JOYCORE-FW` - Fixed device signature
3. `4A4F5943` - Fixed magic number in hex
4. `<FIRMWARE_VERSION>` - Current firmware version (may change between releases)

### Example Response
```
JOYCORE_ID:JOYCORE-FW:4A4F5943:12
```

## Configuration Program Implementation

### Python Example
```python
import serial
import serial.tools.list_ports
import time

def find_joycore_devices():
    """
    Scan all serial ports and identify JoyCore-FW devices.
    Returns a list of (port, firmware_version) tuples.
    """
    joycore_devices = []
    
    # Get all available serial ports
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        try:
            # Open serial connection
            ser = serial.Serial(port.device, 115200, timeout=1)
            time.sleep(0.1)  # Small delay for port stability
            
            # Clear any pending data
            ser.reset_input_buffer()
            
            # Send IDENTIFY command
            ser.write(b"IDENTIFY\n")
            
            # Read response
            response = ser.readline().decode('utf-8').strip()
            
            # Check if this is a JoyCore device
            if response.startswith("JOYCORE_ID:JOYCORE-FW:4A4F5943:"):
                # Extract firmware version
                parts = response.split(':')
                if len(parts) >= 4:
                    firmware_version = parts[3]
                    joycore_devices.append((port.device, firmware_version))
                    print(f"Found JoyCore-FW device on {port.device} (FW v{firmware_version})")
            
            ser.close()
            
        except (serial.SerialException, OSError):
            # Port might be in use or not accessible
            continue
    
    return joycore_devices

# Example usage
if __name__ == "__main__":
    devices = find_joycore_devices()
    if devices:
        print(f"\nFound {len(devices)} JoyCore-FW device(s):")
        for port, fw_version in devices:
            print(f"  - {port}: Firmware v{fw_version}")
    else:
        print("No JoyCore-FW devices found")
```

### C++ Example
```cpp
#include <iostream>
#include <string>
#include <vector>
#include <serial/serial.h>  // Using libserial or similar

struct JoyCoreDevice {
    std::string port;
    int firmwareVersion;
};

std::vector<JoyCoreDevice> findJoyCoreDevices() {
    std::vector<JoyCoreDevice> devices;
    
    // Get list of serial ports (platform-specific)
    std::vector<std::string> ports = getSerialPorts();
    
    for (const auto& portName : ports) {
        try {
            serial::Serial ser(portName, 115200, serial::Timeout::simpleTimeout(1000));
            
            // Send IDENTIFY command
            ser.write("IDENTIFY\n");
            
            // Read response
            std::string response = ser.readline();
            
            // Check for JoyCore signature
            const std::string expectedPrefix = "JOYCORE_ID:JOYCORE-FW:4A4F5943:";
            if (response.find(expectedPrefix) == 0) {
                // Extract firmware version
                std::string fwStr = response.substr(expectedPrefix.length());
                int fwVersion = std::stoi(fwStr);
                
                devices.push_back({portName, fwVersion});
                std::cout << "Found JoyCore-FW on " << portName 
                         << " (FW v" << fwVersion << ")" << std::endl;
            }
            
            ser.close();
        } catch (...) {
            // Port not accessible, continue scanning
        }
    }
    
    return devices;
}
```

## Important Notes

1. **Fixed Response Format**: The first three parts of the response (`JOYCORE_ID:JOYCORE-FW:4A4F5943:`) are guaranteed to never change, making device detection reliable.

2. **Serial Settings**: Always use 115200 baud rate, 8N1 (8 data bits, no parity, 1 stop bit).

3. **Timeout Handling**: Use a reasonable timeout (1 second recommended) when waiting for responses.

4. **Port Stability**: Add a small delay (100ms) after opening the serial port before sending commands.

5. **Error Handling**: Some ports may be in use or inaccessible. Always handle exceptions gracefully and continue scanning other ports.

6. **Firmware Version**: The firmware version field can be used to check compatibility with your configuration program.

## Testing the Protocol

You can test the identification protocol manually using any serial terminal:

1. Connect to the JoyCore-FW device at 115200 baud
2. Send: `IDENTIFY` (followed by Enter/newline)
3. Expected response: `JOYCORE_ID:JOYCORE-FW:4A4F5943:12` (or current firmware version)

## Advantages of This Approach

- **VID/PID Independent**: Works regardless of user USB configuration
- **Simple**: Single command, single response
- **Reliable**: Fixed identifiers ensure consistent detection
- **Fast**: Quick serial query during port scanning
- **Version Aware**: Includes firmware version for compatibility checking
- **Cross-Platform**: Works on Windows, Linux, and macOS

## Future Compatibility

The protocol is designed to be forward-compatible:
- The fixed parts will never change
- Additional fields may be added after the firmware version (separated by colons)
- Config programs should parse only the fields they need and ignore extras