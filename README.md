# GNGR-ButtonBox

A 32-button USB game controller for flight simulators and gaming setups. This Arduino-based project handles multiple input types including matrix buttons, rotary encoders, direct pin buttons, and shift register inputs through a single USB interface.

## Key Features

The project uses several interesting techniques for embedded development:

- **Custom Pin Read Functions**: Function pointers enable the same encoder library to work with direct pins, matrix positions, and shift register bits
- **Union-based Configuration**: Memory-efficient logical input definitions using C++ unions to handle different input types
- **Quadrature Decoding**: Custom state machine for shift register encoders that handles timing issues with polled inputs
- **Matrix Key Scanning**: Efficient button matrix implementation using row/column multiplexing
- **USB HID Integration**: Native USB game controller interface using Arduino's Joystick library

## Technologies and Libraries

- **Arduino Framework**: Core embedded development platform
- **USB HID (Joystick)**: [Arduino Joystick Library](https://github.com/MHeironimus/ArduinoJoystickLibrary) for USB game controller interface
- **Keypad Library**: Matrix scanning for efficient button input handling
- **RotaryEncoder Library**: [Matthias Hertel's RotaryEncoder](https://github.com/mathertel/RotaryEncoder) with custom extensions
- **74HC165 Shift Registers**: Serial-to-parallel conversion for additional inputs
- **C++ Templates and Function Pointers**: Advanced C++ features for hardware abstraction

## Project Structure

```
GNGR-ButtonBox/
├── src/
│   ├── RotaryEncoder/
│   └── lib/
├── include/
└── docs/
```

**src/**: Core application code including input handlers, configuration system, and hardware abstraction layers. The [RotaryEncoder/](src/RotaryEncoder/) subdirectory contains the modified encoder library with shift register support.

**include/**: Header files and configuration definitions for the button box hardware setup.

The project demonstrates advanced embedded C++ patterns including hardware abstraction through function pointers, memory-efficient configuration using unions, and custom quadrature decoding algorithms for handling timing-sensitive encoder inputs through shift registers.

Key files include [Config.h](src/Config.h) for the type system and [UserConfig.h](src/UserConfig.h) for hardware pin mapping. The modular design separates concerns between [ButtonInput.cpp](src/ButtonInput.cpp), [EncoderInput.cpp](src/EncoderInput.cpp), and [MatrixInput.cpp](src/MatrixInput.cpp) for different input types.