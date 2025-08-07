#!/usr/bin/env python3
"""
Read JoyCore configuration via USB HID protocol using pywinusb (Windows-specific)
"""

import pywinusb.hid as hid
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

class JoyCoreConfig:
    def __init__(self):
        self.device = None
        self.response_data = None
        
    def find_device(self):
        """Find JoyCore device"""
        VID = 0x2E8A
        PID = 0xA02F
        
        all_devices = hid.HidDeviceFilter(vendor_id=VID, product_id=PID).get_devices()
        
        if not all_devices:
            print(f"No devices found with VID={VID:04X}, PID={PID:04X}")
            return False
            
        print(f"Found {len(all_devices)} device(s)")
        
        for i, device in enumerate(all_devices):
            print(f"\nDevice {i}:")
            print(f"  Product: {device.product_name}")
            print(f"  Vendor: {device.vendor_name}")
            print(f"  Path: {device.device_path}")
            
            # Open device temporarily to check capabilities
            try:
                device.open()
                print("  Reports:")
                
                # Check input reports
                input_reports = device.find_input_reports()
                if input_reports:
                    print(f"    Input Reports: {len(input_reports)}")
                    for report in input_reports:
                        print(f"      ID: {report.report_id}, Size: {len(report)}")
                
                # Check output reports
                output_reports = device.find_output_reports()
                if output_reports:
                    print(f"    Output Reports: {len(output_reports)}")
                    for report in output_reports:
                        print(f"      ID: {report.report_id}, Size: {len(report)}")
                
                # Check feature reports
                feature_reports = device.find_feature_reports()
                if feature_reports:
                    print(f"    Feature Reports: {len(feature_reports)}")
                    for report in feature_reports:
                        print(f"      ID: {report.report_id}, Size: {len(report)}")
                else:
                    print("    No feature reports found!")
                    
                device.close()
            except Exception as e:
                print(f"  Could not open device: {e}")
                
        # Use the device with feature reports (should be col02)
        for device in all_devices:
            try:
                device.open()
                feature_reports = device.find_feature_reports()
                if feature_reports:
                    print(f"  Using device with feature reports: {device.device_path}")
                    device.close()
                    self.device = device
                    return True
                device.close()
            except:
                pass
                
        # Fallback to first device
        self.device = all_devices[0]
        return True
        
    def open_device(self):
        """Open the device"""
        try:
            self.device.open()
            print(f"\nOpened device: {self.device.product_name}")
            
            # Set raw data handler
            self.device.set_raw_data_handler(self.raw_handler)
            return True
        except Exception as e:
            print(f"Failed to open device: {e}")
            return False
            
    def close_device(self):
        """Close the device"""
        if self.device:
            self.device.close()
            
    def raw_handler(self, data):
        """Handle incoming data"""
        print(f"Received data: {len(data)} bytes")
        self.response_data = data
        
    def create_message(self, msg_type, data=b''):
        """Create a config message"""
        message = [0] * CONFIG_USB_MAX_PACKET_SIZE
        message[0] = CONFIG_USB_FEATURE_REPORT_ID
        message[1] = msg_type
        message[2] = 0  # sequence
        message[3] = 1  # totalPackets
        
        # Pack data length
        data_len = len(data)
        message[4] = data_len & 0xFF
        message[5] = (data_len >> 8) & 0xFF
        
        message[6] = 0  # status
        message[7] = 0  # reserved
        
        # Copy data
        for i in range(min(len(data), 56)):
            message[8 + i] = data[i]
            
        return message
        
    def send_command(self, msg_type):
        """Send a command and get response"""
        # Find the feature report
        reports = self.device.find_feature_reports()
        
        feature_report = None
        for report in reports:
            if report.report_id == CONFIG_USB_FEATURE_REPORT_ID:
                feature_report = report
                break
                
        if not feature_report:
            print(f"No feature report found with ID {CONFIG_USB_FEATURE_REPORT_ID}")
            # Try to create one
            feature_report = self.device.find_feature_reports()[0] if reports else None
            
        if not feature_report:
            print("No feature reports available")
            return None
            
        print(f"Using feature report ID {feature_report.report_id}, size {len(feature_report)}")
        
        # Create message
        message = self.create_message(msg_type)
        
        # Send the message
        try:
            # Set the data in the report
            for i in range(min(len(message), len(feature_report))):
                feature_report[i] = message[i]
                
            feature_report.send()
            print(f"Sent command {msg_type}")
            
            # Wait for response
            time.sleep(0.2)
            
            # Get the feature report
            feature_report.get()
            
            # Extract response
            response = bytes(feature_report[:CONFIG_USB_MAX_PACKET_SIZE])
            return response
            
        except Exception as e:
            print(f"Error sending command: {e}")
            import traceback
            traceback.print_exc()
            return None
            
    def parse_status(self, data):
        """Parse config status"""
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

def main():
    config = JoyCoreConfig()
    
    if not config.find_device():
        return
        
    if not config.open_device():
        return
        
    try:
        # Get status
        print("\n" + "="*60)
        print("Getting Configuration Status...")
        print("="*60)
        
        response = config.send_command(ConfigMessageType.GET_CONFIG_STATUS)
        if response:
            print(f"Response: {response[:20].hex()}")
            
            # Skip message header (8 bytes)
            if len(response) >= 8:
                msg_type = response[1]
                status_code = response[6]
                data_len = struct.unpack('<H', response[4:6])[0]
                
                print(f"Message Type: {msg_type}")
                print(f"Status Code: {status_code}")
                print(f"Data Length: {data_len}")
                
                if status_code == 0 and data_len > 0:
                    status_data = response[8:8+data_len]
                    status = config.parse_status(status_data)
                    if status:
                        print(f"\nConfiguration Status:")
                        print(f"  Mode: {status['modeName']} ({status['currentMode']})")
                        print(f"  Config Loaded: {status['configLoaded']}")
                        print(f"  Using Defaults: {status['usingDefaults']}")
                        print(f"  Storage Initialized: {status['storageInitialized']}")
                        
        # Get config
        print("\n" + "="*60)
        print("Getting Configuration...")
        print("="*60)
        
        response = config.send_command(ConfigMessageType.GET_CONFIG)
        if response:
            print(f"Response first 20 bytes: {response[:20].hex()}")
            
    finally:
        config.close_device()

if __name__ == "__main__":
    main()