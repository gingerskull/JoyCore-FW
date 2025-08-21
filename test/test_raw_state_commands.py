#!/usr/bin/env python3
"""
Test script for JoyCore-FW raw state monitoring commands.

This script tests all the new raw state reading functionality:
- READ_GPIO_STATES
- READ_MATRIX_STATE  
- READ_SHIFT_REG
- START_RAW_MONITOR
- STOP_RAW_MONITOR

Usage:
    python test_raw_state_commands.py [port]

If no port is specified, it will auto-detect the first available COM port.
"""

import serial
import time
import sys
import re
import threading
from datetime import datetime

class RawStateTestManager:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.monitoring_active = False
        self.monitor_thread = None
        self.test_results = {
            'gpio_states': [],
            'matrix_states': [],
            'shift_reg_states': [],
            'monitor_data': []
        }
        
    def find_serial_port(self):
        """Auto-detect the JoyCore device."""
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        
        for port in ports:
            try:
                # Try to connect and send IDENTIFY command
                test_ser = serial.Serial(port.device, self.baudrate, timeout=2)
                test_ser.write(b'IDENTIFY\n')
                time.sleep(0.1)
                response = test_ser.readline().decode('utf-8', errors='ignore').strip()
                test_ser.close()
                
                if 'JoyCore' in response or 'DEVICE_ID' in response:
                    print(f"Found JoyCore device on {port.device}")
                    return port.device
            except:
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
            print(f"Connected to JoyCore on {self.port}")
            
            # Clear any pending data
            self.ser.flushInput()
            
            # Test basic communication
            self.send_command('IDENTIFY')
            response = self.read_response()
            print(f"Device response: {response}")
            
        except Exception as e:
            raise Exception(f"Failed to connect to {self.port}: {e}")
            
    def disconnect(self):
        """Disconnect from the device."""
        if self.monitoring_active:
            self.stop_monitor()
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Disconnected from device")
            
    def send_command(self, command):
        """Send a command to the device."""
        if not self.ser or not self.ser.is_open:
            raise Exception("Not connected to device")
        
        self.ser.write(f"{command}\n".encode())
        self.ser.flush()
        
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
        print("\n=== Testing GPIO States ===")
        
        self.send_command('READ_GPIO_STATES')
        response = self.read_response()
        
        if response:
            print(f"Response: {response}")
            
            # Parse GPIO response: GPIO_STATES:0x[hex]:[timestamp]
            match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', response)
            if match:
                gpio_mask = int(match.group(1), 16)
                timestamp = int(match.group(2))
                
                print(f"GPIO Mask: 0x{gpio_mask:08X}")
                print(f"Timestamp: {timestamp}")
                
                # Show individual pin states
                print("Pin states:")
                for pin in range(30):
                    state = "HIGH" if (gpio_mask & (1 << pin)) else "LOW"
                    print(f"  GPIO{pin:2d}: {state}")
                    
                self.test_results['gpio_states'].append({
                    'mask': gpio_mask,
                    'timestamp': timestamp,
                    'response': response
                })
                return True
            else:
                print(f"Failed to parse response: {response}")
                return False
        else:
            print("No response received")
            return False
            
    def test_matrix_state(self):
        """Test READ_MATRIX_STATE command."""
        print("\n=== Testing Matrix State ===")
        
        self.send_command('READ_MATRIX_STATE')
        responses = self.read_multiple_responses()
        
        if responses:
            print(f"Received {len(responses)} matrix responses:")
            
            matrix_data = {}
            for response in responses:
                print(f"  {response}")
                
                if response == "MATRIX_STATE:NO_MATRIX_CONFIGURED":
                    print("No matrix configured")
                    return True
                elif response == "MATRIX_STATE:NO_MATRIX_PINS_CONFIGURED":
                    print("No matrix pins configured")
                    return True
                    
                # Parse matrix response: MATRIX_STATE:[row]:[col]:[state]:[timestamp]
                match = re.match(r'MATRIX_STATE:(\d+):(\d+):([01]):(\d+)', response)
                if match:
                    row = int(match.group(1))
                    col = int(match.group(2))
                    state = int(match.group(3))
                    timestamp = int(match.group(4))
                    
                    if row not in matrix_data:
                        matrix_data[row] = {}
                    matrix_data[row][col] = state
                    
            if matrix_data:
                print("\nMatrix state grid:")
                for row in sorted(matrix_data.keys()):
                    row_states = []
                    for col in sorted(matrix_data[row].keys()):
                        row_states.append(str(matrix_data[row][col]))
                    print(f"  Row {row}: {' '.join(row_states)}")
                    
            self.test_results['matrix_states'].append({
                'data': matrix_data,
                'responses': responses
            })
            return True
        else:
            print("No response received")
            return False
            
    def test_shift_reg_state(self):
        """Test READ_SHIFT_REG command."""
        print("\n=== Testing Shift Register State ===")
        
        self.send_command('READ_SHIFT_REG')
        responses = self.read_multiple_responses()
        
        if responses:
            print(f"Received {len(responses)} shift register responses:")
            
            shift_data = {}
            for response in responses:
                print(f"  {response}")
                
                if response == "SHIFT_REG:NO_SHIFT_REG_CONFIGURED":
                    print("No shift registers configured")
                    return True
                    
                # Parse shift reg response: SHIFT_REG:[reg_id]:[hex]:[timestamp]
                match = re.match(r'SHIFT_REG:(\d+):0x([0-9A-F]+):(\d+)', response)
                if match:
                    reg_id = int(match.group(1))
                    reg_value = int(match.group(2), 16)
                    timestamp = int(match.group(3))
                    
                    shift_data[reg_id] = reg_value
                    
                    print(f"    Register {reg_id}: 0x{reg_value:02X} (binary: {reg_value:08b})")
                    
            self.test_results['shift_reg_states'].append({
                'data': shift_data,
                'responses': responses
            })
            return True
        else:
            print("No response received")
            return False
            
    def monitor_thread_func(self):
        """Background thread to capture monitoring data."""
        print("Monitoring thread started...")
        
        while self.monitoring_active:
            try:
                line = self.read_response(0.1)
                if line:
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    print(f"[{timestamp}] {line}")
                    self.test_results['monitor_data'].append({
                        'timestamp': timestamp,
                        'data': line
                    })
            except:
                break
                
        print("Monitoring thread stopped")
        
    def test_monitoring(self, duration=10):
        """Test START_RAW_MONITOR and STOP_RAW_MONITOR commands."""
        print(f"\n=== Testing Raw Monitoring (for {duration} seconds) ===")
        
        # Start monitoring
        self.send_command('START_RAW_MONITOR')
        response = self.read_response()
        
        if response != "OK:RAW_MONITOR_STARTED":
            print(f"Failed to start monitoring: {response}")
            return False
            
        print("Monitoring started successfully")
        
        # Start background thread to capture data
        self.monitoring_active = True
        self.monitor_thread = threading.Thread(target=self.monitor_thread_func)
        self.monitor_thread.start()
        
        # Wait for specified duration
        print(f"Collecting data for {duration} seconds...")
        time.sleep(duration)
        
        # Stop monitoring
        self.send_command('STOP_RAW_MONITOR')
        response = self.read_response()
        
        if response != "OK:RAW_MONITOR_STOPPED":
            print(f"Warning: Unexpected stop response: {response}")
            
        print("Monitoring stopped")
        
        # Stop background thread
        self.monitoring_active = False
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2)
            
        return True
        
    def run_all_tests(self):
        """Run all raw state tests."""
        print("Starting JoyCore Raw State Tests")
        print("=" * 50)
        
        results = {
            'gpio': False,
            'matrix': False,
            'shift_reg': False,
            'monitoring': False
        }
        
        try:
            self.connect()
            
            # Test individual commands
            results['gpio'] = self.test_gpio_states()
            results['matrix'] = self.test_matrix_state()
            results['shift_reg'] = self.test_shift_reg_state()
            
            # Test monitoring
            results['monitoring'] = self.test_monitoring(duration=5)
            
        except Exception as e:
            print(f"Test error: {e}")
        finally:
            self.disconnect()
            
        # Print results summary
        print("\n" + "=" * 50)
        print("TEST RESULTS SUMMARY")
        print("=" * 50)
        
        for test, passed in results.items():
            status = "PASS" if passed else "FAIL"
            print(f"{test.upper():15}: {status}")
            
        # Print data summary
        print(f"\nData collected:")
        print(f"  GPIO readings: {len(self.test_results['gpio_states'])}")
        print(f"  Matrix readings: {len(self.test_results['matrix_states'])}")
        print(f"  Shift reg readings: {len(self.test_results['shift_reg_states'])}")
        print(f"  Monitor samples: {len(self.test_results['monitor_data'])}")
        
        return all(results.values())

def main():
    """Main test function."""
    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
        
    print("JoyCore Raw State Commands Test")
    print("=" * 40)
    
    if port:
        print(f"Using specified port: {port}")
    else:
        print("Auto-detecting device...")
        
    tester = RawStateTestManager(port)
    
    try:
        success = tester.run_all_tests()
        exit_code = 0 if success else 1
        
        print(f"\nTest completed with exit code: {exit_code}")
        sys.exit(exit_code)
        
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        tester.disconnect()
        sys.exit(1)
    except Exception as e:
        print(f"\nTest failed with error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()