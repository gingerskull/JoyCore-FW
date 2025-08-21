#!/usr/bin/env python3
"""
Interactive test tool for JoyCore-FW raw state monitoring.

This script provides an interactive menu to test individual raw state commands
and observe real-time hardware states.

Usage:
    python interactive_raw_state_test.py [port]
"""

import serial
import time
import sys
import threading
import re
from datetime import datetime

class InteractiveRawStateTester:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.monitoring_active = False
        self.monitor_thread = None
        
    def find_serial_port(self):
        """Auto-detect the JoyCore device."""
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        
        print("Scanning for JoyCore device...")
        for port in ports:
            try:
                print(f"  Trying {port.device}...")
                test_ser = serial.Serial(port.device, self.baudrate, timeout=2)
                test_ser.write(b'IDENTIFY\n')
                time.sleep(0.1)
                response = test_ser.readline().decode('utf-8', errors='ignore').strip()
                test_ser.close()
                
                if 'JoyCore' in response or 'DEVICE_ID' in response:
                    print(f"  ✓ Found JoyCore device on {port.device}")
                    return port.device
                else:
                    print(f"  ✗ Not JoyCore: {response}")
            except Exception as e:
                print(f"  ✗ Error: {e}")
                continue
                
        return None
        
    def connect(self):
        """Connect to the JoyCore device."""
        if not self.port:
            self.port = self.find_serial_port()
            if not self.port:
                raise Exception("Could not find JoyCore device. Please specify port manually.")
        
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=5)
            time.sleep(2)  # Wait for device to be ready
            print(f"\n✓ Connected to JoyCore on {self.port}")
            
            # Clear any pending data
            self.ser.flushInput()
            
            # Test basic communication
            self.send_command('IDENTIFY')
            response = self.read_response()
            print(f"Device: {response}")
            
            return True
            
        except Exception as e:
            print(f"✗ Failed to connect to {self.port}: {e}")
            return False
            
    def disconnect(self):
        """Disconnect from the device."""
        if self.monitoring_active:
            self.stop_monitoring()
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Disconnected from device")
            
    def send_command(self, command):
        """Send a command to the device."""
        if not self.ser or not self.ser.is_open:
            print("Error: Not connected to device")
            return False
        
        print(f"Sending: {command}")
        self.ser.write(f"{command}\n".encode())
        self.ser.flush()
        return True
        
    def read_response(self, timeout=2):
        """Read a single response line."""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    return line
        return None
        
    def read_multiple_responses(self, timeout=3):
        """Read multiple response lines until timeout."""
        responses = []
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            line = self.read_response(0.1)
            if line:
                responses.append(line)
                start_time = time.time()  # Reset timeout on new data
            else:
                if responses:  # If we got some data, short timeout is OK
                    break
                    
        return responses
        
    def test_gpio_states(self):
        """Test READ_GPIO_STATES command."""
        print("\n--- GPIO States Test ---")
        
        if not self.send_command('READ_GPIO_STATES'):
            return
            
        response = self.read_response()
        
        if response:
            print(f"Response: {response}")
            
            # Parse GPIO response: GPIO_STATES:0x[hex]:[timestamp]
            match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', response)
            if match:
                gpio_mask = int(match.group(1), 16)
                timestamp = int(match.group(2))
                
                print(f"GPIO Mask: 0x{gpio_mask:08X}")
                print(f"Timestamp: {timestamp} μs")
                
                # Show individual pin states (only show pins that are HIGH to reduce clutter)
                high_pins = []
                for pin in range(30):
                    if gpio_mask & (1 << pin):
                        high_pins.append(str(pin))
                        
                if high_pins:
                    print(f"HIGH pins: {', '.join(high_pins)}")
                else:
                    print("All pins are LOW")
                    
            else:
                print(f"Failed to parse response: {response}")
        else:
            print("No response received")
            
    def test_matrix_state(self):
        """Test READ_MATRIX_STATE command."""
        print("\n--- Matrix State Test ---")
        
        if not self.send_command('READ_MATRIX_STATE'):
            return
            
        responses = self.read_multiple_responses()
        
        if responses:
            print(f"Received {len(responses)} responses:")
            
            matrix_data = {}
            for response in responses:
                print(f"  {response}")
                
                if "NO_MATRIX" in response:
                    return
                    
                # Parse matrix response: MATRIX_STATE:[row]:[col]:[state]:[timestamp]
                match = re.match(r'MATRIX_STATE:(\d+):(\d+):([01]):(\d+)', response)
                if match:
                    row = int(match.group(1))
                    col = int(match.group(2))
                    state = int(match.group(3))
                    
                    if row not in matrix_data:
                        matrix_data[row] = {}
                    matrix_data[row][col] = state
                    
            if matrix_data:
                print("\nMatrix Grid (1=pressed, 0=released):")
                print("   ", end="")
                max_col = max(max(row.keys()) for row in matrix_data.values())
                for col in range(max_col + 1):
                    print(f"{col:2}", end="")
                print()
                
                for row in sorted(matrix_data.keys()):
                    print(f"R{row}: ", end="")
                    for col in range(max_col + 1):
                        state = matrix_data[row].get(col, 0)
                        print(f"{state:2}", end="")
                    print()
        else:
            print("No response received")
            
    def test_shift_reg_state(self):
        """Test READ_SHIFT_REG command."""
        print("\n--- Shift Register State Test ---")
        
        if not self.send_command('READ_SHIFT_REG'):
            return
            
        responses = self.read_multiple_responses()
        
        if responses:
            print(f"Received {len(responses)} responses:")
            
            for response in responses:
                print(f"  {response}")
                
                if "NO_SHIFT_REG" in response:
                    return
                    
                # Parse shift reg response: SHIFT_REG:[reg_id]:[hex]:[timestamp]
                match = re.match(r'SHIFT_REG:(\d+):0x([0-9A-F]+):(\d+)', response)
                if match:
                    reg_id = int(match.group(1))
                    reg_value = int(match.group(2), 16)
                    timestamp = int(match.group(3))
                    
                    print(f"    Register {reg_id}: 0x{reg_value:02X}")
                    print(f"    Binary:     {reg_value:08b}")
                    
                    # Show which bits are set
                    set_bits = []
                    for bit in range(8):
                        if reg_value & (1 << bit):
                            set_bits.append(str(bit))
                    
                    if set_bits:
                        print(f"    Set bits:   {', '.join(set_bits)}")
                    else:
                        print(f"    All bits clear")
        else:
            print("No response received")
            
    def monitor_thread_func(self):
        """Background thread to capture monitoring data."""
        line_count = 0
        while self.monitoring_active:
            try:
                line = self.read_response(0.1)
                if line:
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    print(f"\r[{timestamp}] {line}")
                    line_count += 1
                    if line_count % 10 == 0:
                        print(f"(Received {line_count} updates so far...)")
            except:
                break
                
    def start_monitoring(self):
        """Start continuous monitoring."""
        print("\n--- Starting Raw State Monitoring ---")
        
        if not self.send_command('START_RAW_MONITOR'):
            return False
            
        response = self.read_response()
        
        if response == "OK:RAW_MONITOR_STARTED":
            print("✓ Monitoring started successfully")
            print("Press Ctrl+C to stop monitoring\n")
            
            # Start background thread to capture data
            self.monitoring_active = True
            self.monitor_thread = threading.Thread(target=self.monitor_thread_func)
            self.monitor_thread.start()
            
            return True
        else:
            print(f"✗ Failed to start monitoring: {response}")
            return False
            
    def stop_monitoring(self):
        """Stop continuous monitoring."""
        if not self.monitoring_active:
            return
            
        print("\nStopping monitoring...")
        
        # Stop background thread first
        self.monitoring_active = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2)
            
        # Send stop command
        if self.send_command('STOP_RAW_MONITOR'):
            response = self.read_response()
            if response == "OK:RAW_MONITOR_STOPPED":
                print("✓ Monitoring stopped")
            else:
                print(f"Warning: Unexpected response: {response}")
                
    def show_menu(self):
        """Display the interactive menu."""
        print("\n" + "=" * 50)
        print("JoyCore Raw State Interactive Tester")
        print("=" * 50)
        print("1. Test GPIO States")
        print("2. Test Matrix State")
        print("3. Test Shift Register State")
        print("4. Start Continuous Monitoring")
        print("5. Send Custom Command")
        print("0. Quit")
        print("-" * 50)
        
    def run_interactive(self):
        """Run the interactive test menu."""
        if not self.connect():
            return False
            
        try:
            while True:
                self.show_menu()
                choice = input("Enter choice (0-5): ").strip()
                
                if choice == '0':
                    print("Goodbye!")
                    break
                elif choice == '1':
                    self.test_gpio_states()
                elif choice == '2':
                    self.test_matrix_state()
                elif choice == '3':
                    self.test_shift_reg_state()
                elif choice == '4':
                    if self.start_monitoring():
                        try:
                            # Wait for user to stop
                            while self.monitoring_active:
                                time.sleep(0.1)
                        except KeyboardInterrupt:
                            self.stop_monitoring()
                elif choice == '5':
                    command = input("Enter command: ").strip()
                    if command:
                        self.send_command(command)
                        responses = self.read_multiple_responses()
                        if responses:
                            for response in responses:
                                print(f"  {response}")
                        else:
                            print("  No response")
                else:
                    print("Invalid choice")
                    
                input("\nPress Enter to continue...")
                
        except KeyboardInterrupt:
            print("\nInterrupted by user")
        finally:
            self.disconnect()
            
        return True

def main():
    """Main function."""
    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
        
    print("JoyCore Raw State Interactive Tester")
    print("=" * 40)
    
    if port:
        print(f"Using specified port: {port}")
    else:
        print("Will auto-detect device...")
        
    tester = InteractiveRawStateTester(port)
    
    try:
        tester.run_interactive()
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()