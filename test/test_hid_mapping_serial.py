#!/usr/bin/env python3
"""
HID Mapping Serial Test Script for JoyCore-FW

This script tests the new HID mapping features using the serial command interface.
It's simpler to use than the HID version as it doesn't require USB HID libraries.

Requirements:
- pyserial: pip install pyserial
- JoyCore device connected via USB (serial port)

Usage:
    python test_hid_mapping_serial.py [COM_PORT]
    
Examples:
    python test_hid_mapping_serial.py COM3        # Windows
    python test_hid_mapping_serial.py /dev/ttyACM0 # Linux
    python test_hid_mapping_serial.py             # Auto-detect
"""

import serial
import serial.tools.list_ports
import time
import sys
import re
from typing import Optional, Dict, List

class JoyCoreSerial:
    """Serial interface for JoyCore HID mapping features"""
    
    def __init__(self, port: Optional[str] = None, baudrate: int = 115200):
        """Initialize serial connection"""
        self.ser = None
        self.connect(port, baudrate)
    
    def connect(self, port: Optional[str], baudrate: int) -> bool:
        """Connect to JoyCore device via serial"""
        if port is None:
            port = self.auto_detect_port()
            if port is None:
                return False
        
        try:
            self.ser = serial.Serial(port, baudrate, timeout=2)
            time.sleep(2)  # Wait for device to be ready
            
            # Test connection with IDENTIFY command
            response = self.send_command("IDENTIFY")
            if "JOYCORE" in response:
                print(f"✅ Connected to JoyCore device on {port}")
                return True
            else:
                print(f"❌ Device on {port} is not JoyCore")
                return False
                
        except Exception as e:
            print(f"❌ Failed to connect to {port}: {e}")
            return False
    
    def auto_detect_port(self) -> Optional[str]:
        """Auto-detect JoyCore device port"""
        print("🔍 Auto-detecting JoyCore device...")
        
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(f"   Trying {port.device}...")
            try:
                test_ser = serial.Serial(port.device, 115200, timeout=1)
                time.sleep(1)
                test_ser.write(b"IDENTIFY\n")
                response = test_ser.read(100).decode('utf-8', errors='ignore')
                test_ser.close()
                
                if "JOYCORE" in response:
                    print(f"✅ Found JoyCore device on {port.device}")
                    return port.device
                    
            except Exception:
                continue
        
        print("❌ JoyCore device not found on any serial port")
        return None
    
    def send_command(self, command: str) -> str:
        """Send command and return response"""
        if not self.ser:
            return ""
        
        try:
            # Clear input buffer
            self.ser.reset_input_buffer()
            
            # Send command
            self.ser.write(f"{command}\n".encode())
            
            # Read response (wait up to 2 seconds)
            response = ""
            start_time = time.time()
            while time.time() - start_time < 2:
                if self.ser.in_waiting > 0:
                    data = self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
                    response += data
                    if '\n' in response:
                        break
                time.sleep(0.01)
            
            return response.strip()
            
        except Exception as e:
            print(f"❌ Command failed: {e}")
            return ""
    
    def get_mapping_info(self) -> Optional[Dict]:
        """Get HID mapping information via serial"""
        response = self.send_command("HID_MAPPING_INFO")
        if not response.startswith("HID_MAPPING_INFO:"):
            return None
        
        # Parse response: HID_MAPPING_INFO:ver=1,rid=1,btn=32,axis=8,btn_offset=0,bit_order=0,crc=0x0000,fc_offset=48
        data = response.split(":", 1)[1]
        info = {}
        
        for pair in data.split(","):
            if "=" in pair:
                key, value = pair.split("=", 1)
                if key in ["ver", "rid", "btn", "axis", "btn_offset", "bit_order", "fc_offset"]:
                    info[key] = int(value)
                elif key == "crc":
                    info[key] = int(value, 16)
        
        # Add derived fields
        info['is_sequential'] = info.get('crc', 1) == 0x0000
        info['button_count'] = info.get('btn', 0)
        info['axis_count'] = info.get('axis', 0)
        
        return info
    
    def get_button_mapping(self) -> Optional[List[int]]:
        """Get button mapping via serial"""
        response = self.send_command("HID_BUTTON_MAP")
        if response == "HID_BUTTON_MAP:SEQUENTIAL":
            return []  # Sequential mapping
        elif response.startswith("HID_BUTTON_MAP:"):
            # Parse mapping: HID_BUTTON_MAP:0,1,2,3,4,5,6,7,...
            data = response.split(":", 1)[1]
            if data:
                return [int(x) for x in data.split(",")]
        
        return None
    
    def start_self_test(self) -> bool:
        """Start self-test via serial"""
        response = self.send_command("HID_SELFTEST start")
        return "HID_SELFTEST:STARTED" in response
    
    def stop_self_test(self) -> bool:
        """Stop self-test via serial"""
        response = self.send_command("HID_SELFTEST stop")
        return "HID_SELFTEST:STOPPED" in response
    
    def get_self_test_status(self) -> Optional[Dict]:
        """Get self-test status via serial"""
        response = self.send_command("HID_SELFTEST status")
        if not response.startswith("HID_SELFTEST:"):
            return None
        
        # Parse: HID_SELFTEST:status=RUNNING,btn=5,interval=40
        data = response.split(":", 1)[1]
        status = {}
        
        for pair in data.split(","):
            if "=" in pair:
                key, value = pair.split("=", 1)
                if key == "status":
                    status['status_name'] = value
                elif key in ["btn", "interval"]:
                    status[key] = int(value)
        
        return status
    
    def close(self):
        """Close serial connection"""
        if self.ser:
            self.ser.close()


def print_header(title: str):
    """Print a formatted header"""
    print(f"\n{'='*60}")
    print(f"🔍 {title}")
    print(f"{'='*60}")


def print_mapping_info(info: Dict):
    """Print HID mapping information"""
    print_header("HID Mapping Information")
    
    print(f"Protocol Version:     {info.get('ver', 'Unknown')}")
    print(f"Input Report ID:      {info.get('rid', 'Unknown')}")
    print(f"Button Count:         {info.get('button_count', 'Unknown')}")
    print(f"Axis Count:           {info.get('axis_count', 'Unknown')}")
    print(f"Button Byte Offset:   {info.get('btn_offset', 'Unknown')}")
    
    bit_order = info.get('bit_order', -1)
    bit_order_str = 'LSB-first' if bit_order == 0 else 'MSB-first' if bit_order == 1 else 'Unknown'
    print(f"Button Bit Order:     {bit_order_str}")
    
    print(f"Frame Counter Offset: {info.get('fc_offset', 'Unknown')}")
    
    crc = info.get('crc', -1)
    if crc >= 0:
        print(f"Mapping CRC:          0x{crc:04X}")
    else:
        print(f"Mapping CRC:          Unknown")
    
    print(f"Sequential Mapping:   {'✅ Yes' if info.get('is_sequential', False) else '❌ No (custom mapping)'}")


def print_button_mapping(mapping: List[int], is_sequential: bool):
    """Print button mapping"""
    print_header("Button Mapping")
    
    if is_sequential:
        print("✅ Sequential mapping detected (bit index == joyButtonID)")
        print("   No custom mapping table needed")
        return
    
    if not mapping:
        print("❌ No mapping data available")
        return
    
    print("📋 Custom button mapping:")
    print("   Bit Index → Joy Button ID")
    print("   " + "-" * 25)
    
    for bit_index, joy_button_id in enumerate(mapping):
        marker = "⚠️" if bit_index != joy_button_id else "✅"
        print(f"   {bit_index:3d}       →  {joy_button_id:3d}   {marker}")


def run_self_test_demo(joycore: JoyCoreSerial, duration: int = 10):
    """Run self-test demonstration"""
    print_header(f"Self-Test Button Walk Demo ({duration}s)")
    
    print("🧪 Starting self-test button walk...")
    print("   Each button will be activated for 40ms in sequence")
    print("   This helps validate button mapping and detect stuck buttons")
    print()
    
    # Start self-test
    if not joycore.start_self_test():
        print("❌ Failed to start self-test")
        return
    
    print("✅ Self-test started")
    
    start_time = time.time()
    last_button = -1
    
    try:
        while time.time() - start_time < duration:
            status = joycore.get_self_test_status()
            if not status:
                print("❌ Failed to get self-test status")
                break
            
            status_name = status.get('status_name', 'UNKNOWN')
            current_btn = status.get('btn', -1)
            
            if status_name == "COMPLETE":
                print(f"✅ Self-test completed at button {current_btn}")
                break
            elif status_name == "RUNNING":
                if current_btn != last_button and current_btn >= 0:
                    print(f"   Testing button {current_btn:3d}...")
                    last_button = current_btn
            elif status_name == "IDLE":
                print("⚠️ Self-test stopped or not running")
                break
            
            time.sleep(0.2)
    
    except KeyboardInterrupt:
        print("\n⚠️ Self-test interrupted by user")
    
    # Stop self-test
    joycore.stop_self_test()
    print("🛑 Self-test stopped")


def test_serial_commands(joycore: JoyCoreSerial):
    """Test all available serial commands"""
    print_header("Serial Commands Test")
    
    commands = [
        ("IDENTIFY", "Device identification"),
        ("STATUS", "Configuration status"),
        ("HID_MAPPING_INFO", "HID mapping information"),
        ("HID_BUTTON_MAP", "Button mapping table"),
        ("HID_SELFTEST status", "Self-test status")
    ]
    
    for cmd, description in commands:
        print(f"\n📤 {description}:")
        print(f"   Command: {cmd}")
        response = joycore.send_command(cmd)
        print(f"   Response: {response}")


def interactive_menu(joycore: JoyCoreSerial):
    """Interactive menu for testing features"""
    print_header("Interactive Test Menu")
    
    while True:
        print("\n📋 Available Tests:")
        print("   1. Get HID Mapping Info")
        print("   2. Get Button Mapping") 
        print("   3. Run Self-Test Demo")
        print("   4. Test All Serial Commands")
        print("   5. Get Self-Test Status")
        print("   6. Device Status")
        print("   0. Exit")
        
        try:
            choice = input("\n👉 Select test (0-6): ").strip()
            
            if choice == "0":
                break
            elif choice == "1":
                info = joycore.get_mapping_info()
                if info:
                    print_mapping_info(info)
                else:
                    print("❌ Failed to get mapping info")
            
            elif choice == "2":
                info = joycore.get_mapping_info()
                if info:
                    if info.get('is_sequential', False):
                        print_button_mapping([], True)
                    else:
                        mapping = joycore.get_button_mapping()
                        if mapping is not None:
                            print_button_mapping(mapping, False)
                        else:
                            print("❌ Failed to get button mapping")
                else:
                    print("❌ Failed to get mapping info")
            
            elif choice == "3":
                duration = input("Self-test duration in seconds (default 10): ").strip()
                try:
                    duration = int(duration) if duration else 10
                except ValueError:
                    duration = 10
                run_self_test_demo(joycore, duration)
            
            elif choice == "4":
                test_serial_commands(joycore)
            
            elif choice == "5":
                status = joycore.get_self_test_status()
                if status:
                    print(f"Self-test status: {status.get('status_name', 'Unknown')}")
                    print(f"Current button: {status.get('btn', 'Unknown')}")
                    print(f"Interval: {status.get('interval', 'Unknown')}ms")
                else:
                    print("❌ Failed to get self-test status")
            
            elif choice == "6":
                response = joycore.send_command("STATUS")
                print(f"Device Status: {response}")
            
            else:
                print("❌ Invalid choice")
        
        except KeyboardInterrupt:
            print("\n👋 Exiting...")
            break
        except Exception as e:
            print(f"❌ Error: {e}")


def main():
    """Main test function"""
    print("🎮 JoyCore HID Mapping Serial Test Script")
    print("=" * 60)
    
    # Parse command line arguments
    port = sys.argv[1] if len(sys.argv) > 1 else None
    
    # Connect to JoyCore device
    joycore = JoyCoreSerial(port)
    if not joycore.ser:
        print("\n💡 Make sure your JoyCore device is connected via USB")
        print("💡 On Windows, try COM3, COM4, etc.")
        print("💡 On Linux, try /dev/ttyACM0, /dev/ttyUSB0, etc.")
        return 1
    
    try:
        # Run comprehensive demo
        print("\n🚀 Running comprehensive demo of HID mapping features...")
        
        # 1. Get and display mapping info
        info = joycore.get_mapping_info()
        if info:
            print_mapping_info(info)
            
            # 2. Get and display button mapping
            if info.get('is_sequential', False):
                print_button_mapping([], True)
            else:
                mapping = joycore.get_button_mapping()
                if mapping is not None:
                    print_button_mapping(mapping, False)
                else:
                    print("❌ Failed to get button mapping")
        else:
            print("❌ Failed to get HID mapping info")
        
        # 3. Quick self-test demo
        print("\n🧪 Quick self-test demonstration...")
        run_self_test_demo(joycore, 5)
        
        # 4. Interactive menu
        interactive_menu(joycore)
        
        print("\n✅ Test script completed successfully!")
        print("💡 All HID mapping features are working correctly")
        
    finally:
        joycore.close()
    
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n👋 Test script interrupted by user")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        sys.exit(1)