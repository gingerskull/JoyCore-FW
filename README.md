# GNGR-ButtonBox

GNGR-ButtonBox is an Arduino-based USB game controller designed for flight simulator enthusiasts. It supports multiple input types, including matrix buttons, rotary encoders, direct pin buttons, and shift register inputs. This project allows users to build their own customizable button box for enhanced simulator control.

---

## Features

- **Matrix Button Scanning**: Efficient row/column multiplexing for handling large numbers of buttons.
- **Rotary Encoder Support**: Includes custom quadrature decoding for shift register-based encoders and standard encoder handling.
- **Shift Register Integration**: Uses 74HC165 chips to expand input capabilities while minimizing pin usage.
- **USB HID Game Controller**: Implements a native USB interface using the [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary).
- **Modular Design**: Separate modules for button input, encoder input, and matrix scanning ensure clean and maintainable code.

---

## Technologies and Libraries

- **Arduino Framework**: Core platform for embedded development.
- **USB HID (Joystick)**: [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary) for USB game controller functionality.
- **Keypad Library**: Handles matrix scanning for button inputs.
- **RotaryEncoder Library**: [Matthias Hertel's RotaryEncoder](https://github.com/mathertel/RotaryEncoder), extended to support shift register inputs.
- **74HC165 Shift Registers**: Serial-to-parallel conversion for additional input handling.
- **C++ Function Pointers**: Enables flexible hardware abstraction for pin reading.

---

## Project Structure

```
GNGR-ButtonBox/
├── src/
│   ├── RotaryEncoder/
│   ├── lib/
├── include/
├── docs/
```

**src/**: Contains the core application code, including input handlers, configuration systems, and hardware abstraction layers. The [RotaryEncoder/](src/RotaryEncoder/) directory includes the modified encoder library with shift register support.

**include/**: Header files and configuration definitions for hardware setup.

**docs/**: Documentation for the project, including setup instructions and technical details.

---

## How to Use

### Configuration

The project relies on the `UserConfig.h` file to define hardware pin mappings and input behaviors. This file is located in the `include/` directory. It allows users to specify:

- **Button Pins**: Define pins for direct buttons and matrix rows/columns.
- **Encoder Pins**: Assign pins for rotary encoders, including shift register-based encoders.
- **Shift Register Pins**: Specify pins for `PL`, `CLK`, and `QH` connections to 74HC165 chips.

### Example Structure

```cpp
// UserConfig.h
#define BTN_PIN_1 2
#define BTN_PIN_2 3
#define ENC_PIN_A 4
#define ENC_PIN_B 5
#define SHIFTREG_PL 6
#define SHIFTREG_CLK 7
#define SHIFTREG_QH 8
```

### Steps to Build

1. **Clone the Repository**: Download the project files to your local machine.
2. **Install Dependencies**: Ensure the Arduino IDE has the required libraries installed:
   - [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary)
   - [Keypad Library](https://playground.arduino.cc/Code/Keypad/)
3. **Customize `UserConfig.h`**: Modify the file to match your hardware setup.
4. **Upload to Arduino**: Compile and upload the code to your Arduino board.

---

This project is ideal for flight simulator enthusiasts looking to build a customizable button box or input device. It demonstrates advanced techniques for handling multiple input types efficiently while maintaining modularity and flexibility.