#!/usr/bin/env python3
"""
Real-time raw state monitor for JoyCore-FW.

This script provides a continuous, real-time display of GPIO, matrix, and 
shift register states in a clean dashboard format.

Usage:
    python raw_state_monitor.py [port]
"""

import serial
import time
import sys
import re
import os
import threading
from datetime import datetime

class RawStateMonitor:
    def __init__(self, port=None, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.running = False
        self.monitor_thread = None
        
        # State storage
        self.gpio_state = 0
        self.matrix_state = {}
        self.shift_reg_state = {}
        self.last_update = datetime.now()
        self.update_count = 0
        
        # Display lock for thread safety
        self.display_lock = threading.Lock()
        
    def find_serial_port(self):
        """Auto-detect the JoyCore device."""
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        
        for port in ports:
            try:
                test_ser = serial.Serial(port.device, self.baudrate, timeout=2)
                test_ser.write(b'IDENTIFY\n')
                time.sleep(0.1)
                response = test_ser.readline().decode('utf-8', errors='ignore').strip()
                test_ser.close()
                
                if 'JoyCore' in response or 'DEVICE_ID' in response:
                    return port.device
            except:
                continue
                
        return None
        
    def connect(self):
        """Connect to the JoyCore device."""
        if not self.port:
            self.port = self.find_serial_port()
            if not self.port:
                raise Exception("Could not find JoyCore device")
        
        self.ser = serial.Serial(self.port, self.baudrate, timeout=5)
        time.sleep(2)
        self.ser.flushInput()
        
        # Test connection
        self.ser.write(b'IDENTIFY\n')
        response = self.ser.readline().decode('utf-8', errors='ignore').strip()
        
        if not ('JoyCore' in response or 'DEVICE_ID' in response):
            raise Exception(f"Device not responding correctly: {response}")
            
        print(f"Connected to: {response}")
        
    def start_monitoring(self):
        """Start the monitoring process."""
        print("Starting raw state monitoring...")
        
        # Start monitoring on device
        self.ser.write(b'START_RAW_MONITOR\n')
        response = self.ser.readline().decode('utf-8', errors='ignore').strip()
        
        if response != "OK:RAW_MONITOR_STARTED":
            raise Exception(f"Failed to start monitoring: {response}")
            
        print("Monitoring started. Press Ctrl+C to stop.\n")
        
        # Start background thread
        self.running = True
        self.monitor_thread = threading.Thread(target=self.monitor_loop)
        self.monitor_thread.start()
        
    def stop_monitoring(self):
        """Stop the monitoring process."""
        self.running = False
        
        if self.ser and self.ser.is_open:
            self.ser.write(b'STOP_RAW_MONITOR\n')
            
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2)
            
    def monitor_loop(self):
        """Background monitoring loop."""
        while self.running:
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.parse_state_line(line)
                        with self.display_lock:
                            self.update_display()
                time.sleep(0.01)  # Small delay to prevent CPU spinning
            except:
                break
                
    def parse_state_line(self, line):
        """Parse a state line from the device."""
        self.last_update = datetime.now()
        self.update_count += 1
        
        # Parse GPIO states
        gpio_match = re.match(r'GPIO_STATES:0x([0-9A-F]+):(\d+)', line)
        if gpio_match:
            self.gpio_state = int(gpio_match.group(1), 16)
            return
            
        # Parse matrix states
        matrix_match = re.match(r'MATRIX_STATE:(\d+):(\d+):([01]):(\d+)', line)
        if matrix_match:
            row = int(matrix_match.group(1))
            col = int(matrix_match.group(2))
            state = int(matrix_match.group(3))
            
            if row not in self.matrix_state:
                self.matrix_state[row] = {}
            self.matrix_state[row][col] = state
            return
            
        # Parse shift register states
        shift_match = re.match(r'SHIFT_REG:(\d+):0x([0-9A-F]+):(\d+)', line)
        if shift_match:
            reg_id = int(shift_match.group(1))
            reg_value = int(shift_match.group(2), 16)
            self.shift_reg_state[reg_id] = reg_value
            return
            
    def clear_screen(self):
        """Clear the terminal screen."""
        os.system('cls' if os.name == 'nt' else 'clear')
        
    def update_display(self):
        """Update the display with current states."""
        self.clear_screen()
        
        print("=" * 70)
        print("JoyCore Raw State Monitor")
        print("=" * 70)
        
        # Status info
        print(f"Device: {self.port}")
        print(f"Last Update: {self.last_update.strftime('%H:%M:%S.%f')[:-3]}")
        print(f"Updates Received: {self.update_count}")
        print()
        
        # GPIO States
        print("GPIO STATES (showing HIGH pins only):")
        print("-" * 40)
        high_pins = []
        for pin in range(30):
            if self.gpio_state & (1 << pin):
                high_pins.append(pin)
                
        if high_pins:
            # Group pins for better display
            for i in range(0, len(high_pins), 10):
                pins_group = high_pins[i:i+10]
                pins_str = ', '.join(f"GPIO{pin:2d}" for pin in pins_group)
                print(f"  {pins_str}")
        else:
            print("  All pins LOW")
            
        print(f"  Raw mask: 0x{self.gpio_state:08X}")
        print()
        
        # Matrix States
        if self.matrix_state:
            print("MATRIX STATES:")
            print("-" * 40)
            
            max_row = max(self.matrix_state.keys()) if self.matrix_state else 0
            max_col = max(max(row.keys()) for row in self.matrix_state.values()) if self.matrix_state else 0
            
            # Column headers
            print("     ", end="")
            for col in range(max_col + 1):
                print(f"{col:3}", end="")
            print()
            
            # Matrix grid
            for row in range(max_row + 1):
                print(f"R{row:2}: ", end="")
                for col in range(max_col + 1):
                    state = self.matrix_state.get(row, {}).get(col, 0)
                    symbol = "●" if state else "○"
                    print(f"{symbol:>3}", end="")
                print()
                
            # Pressed buttons summary
            pressed = []
            for row, cols in self.matrix_state.items():
                for col, state in cols.items():
                    if state:
                        pressed.append(f"R{row}C{col}")
                        
            if pressed:
                print(f"  Pressed: {', '.join(pressed)}")
            else:
                print("  No buttons pressed")
        else:
            print("MATRIX STATES: Not configured")
            
        print()
        
        # Shift Register States
        if self.shift_reg_state:
            print("SHIFT REGISTER STATES:")
            print("-" * 40)
            
            for reg_id in sorted(self.shift_reg_state.keys()):
                value = self.shift_reg_state[reg_id]
                print(f"  Register {reg_id}: 0x{value:02X} ({value:08b})")
                
                # Show set bits
                set_bits = []
                for bit in range(8):
                    if value & (1 << bit):
                        set_bits.append(f"B{bit}")
                        
                if set_bits:
                    print(f"    Set bits: {', '.join(set_bits)}")
                else:
                    print(f"    All bits clear")
        else:
            print("SHIFT REGISTERS: Not configured")
            
        print()
        print("Press Ctrl+C to stop monitoring")
        
    def run(self):
        """Run the monitor."""
        try:
            self.connect()
            self.start_monitoring()
            
            # Keep main thread alive
            while self.running:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\nStopping monitor...")
        except Exception as e:
            print(f"Error: {e}")
        finally:
            self.stop_monitoring()
            if self.ser and self.ser.is_open:
                self.ser.close()

def main():
    """Main function."""
    port = None
    if len(sys.argv) > 1:
        port = sys.argv[1]
        
    print("JoyCore Raw State Monitor")
    print("=" * 30)
    
    if port:
        print(f"Using port: {port}")
    else:
        print("Auto-detecting device...")
        
    monitor = RawStateMonitor(port)
    monitor.run()

if __name__ == "__main__":
    main()