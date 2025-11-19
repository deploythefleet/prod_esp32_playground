# Simple Factory Floor Device Provisioning Example

This example illustrates how to use a simple ESP32 console application which interacts with a simple python script to store data, like a model number, on the ESP32 on the factory floor. There are two pieces to this example:

1. The ESP32 application which is just a very simple CLI program that supports the following commands
    - `id`: Get the MAC address of the device
    - `get model`: Return the current model number (stored in memory for demonstration purposes)
    - `set model <model number>`: Sets the model number in memory
    - `restart`: Restarts the ESP32 device
1. A python CLI application (model_programmer.py) which interacts with the ESP32 device to "program" a model number.

# ESP32 Application

The `main.cpp` file demonstrates how to build an interactive console application on ESP32. Here's how it works:

**Setup (in `app_main()`):**
1. Configures UART0 for serial communication with line ending handling (CR for input, CRLF for output)
2. Initializes the ESP-IDF console framework with linenoise for command-line editing (arrow keys, history, etc.)
3. Registers built-in help command and custom commands via `register_commands()`
4. Enters a loop that reads user input and executes commands

**Adding Your Own Commands:**
Each command is a simple C function that takes `argc` and `argv` parameters (like a standard main function). Look at `model_number_cmd()` as an example:
- Parse the arguments to determine what action to take
- Perform the action (read/write data, call APIs, etc.)
- Print output to the user
- Return 0 on success

Register your command by creating an `esp_console_cmd_t` structure with your function pointer, help text, and hint text, then call `esp_console_cmd_register()` in the `register_commands()` function.

**Customization Tips:**
- Change the prompt by modifying `config.prompt = "widget>"` 
- Add more complex commands with argument parsing libraries
- Adjust UART settings (baud rate, etc.) in the `uart_config` structure


# ESP32 Model Number Programmer (Python Script)

A Python tool for automatically programming ESP32 devices with model numbers via USB serial connection.

## Overview

This program monitors for ESP32 devices connecting via USB and automatically programs them with a specified model number. It can handle multiple devices connecting simultaneously and verifies that each device is programmed correctly. The program tracks devices by their MAC address, making it ideal for production environments where USB ports are reused as devices are plugged and unplugged.

## Features

- **Automatic USB Detection**: Continuously monitors for new USB serial devices
- **Concurrent Programming**: Handles multiple devices connecting at the same time
- **MAC Address Tracking**: Uses device MAC addresses (not USB ports) to prevent reprogramming the same device
- **Port Reuse Support**: Designed for USB hubs where ports are reused as operators swap devices
- **Verification**: Confirms model number was set correctly using the `model get` command
- **Session Summary**: Displays all programmed device MAC addresses when the program exits
- **Simple CLI**: Easy command-line interface requiring only the model number

## Requirements

- Python 3.6+
- pyserial library

## Installation

1. Install Python dependencies:
```bash
pip install -r requirements.txt
```

Or install pyserial directly:
```bash
pip install pyserial
```

## Usage

Basic usage:
```bash
python model_programmer.py <model_number>
```

Example:
```bash
python model_programmer.py ABC-123
```

### Command-Line Options

- `model_number` (required): The model number to program into devices
- `-b, --baudrate`: Serial baudrate (default: 115200)
- `-i, --interval`: USB polling interval in seconds (default: 1.0)
- `-d, --debug`: Enable debug output (shows all serial communication and verbose messages)

Example with options:
```bash
python model_programmer.py XYZ-789 --baudrate 115200 --interval 0.5
```

Debug mode example:
```bash
python model_programmer.py ABC-123 --debug
```

## How It Works

1. The program starts monitoring for USB serial devices
2. When a new device is detected on any port, it:
   - Connects to the device's serial port
   - Waits for the `widget>` prompt
   - Sends `id` command to retrieve the device's MAC address
   - Checks if this MAC address has already been programmed in this session
   - If not already programmed:
     - Sends `model set <model_number>` command
     - Verifies with `model get` command
     - Confirms the model number matches
     - Marks the MAC address as programmed
3. The program continues monitoring for new devices
4. When stopped (Ctrl+C), displays a list of all programmed device MAC addresses

### Port Reuse Support

The program tracks devices by MAC address rather than USB port. This means:
- You can unplug a programmed device and plug in a new one on the same port
- The new device will be programmed (if its MAC hasn't been seen before)
- Previously programmed devices won't be reprogrammed if reconnected
- Ideal for production lines with limited USB ports on a hub

### Clean Output for Operators

In normal mode (without `--debug`), the program shows only:
- Successful programming events with MAC addresses
- Errors that require attention
- Final summary of all programmed devices

Already-programmed devices that remain connected are quietly ignored to keep the output clean and easy to read. Use `--debug` mode to see all detailed operations including skipped devices and serial communication.

## ESP32 Console Requirements

The ESP32 device must present a console application with:
- Prompt: `widget>`
- Command: `id` - Returns device information including MAC address in format "MAC: XX:XX:XX:XX:XX:XX"
- Command: `model set <model_number>` - Sets the model number
- Command: `model get` - Returns the current model number
- Default baudrate: 115200

## Example Output

Normal operation (concise, operator-friendly):
```
Monitoring for ESP32 devices to program with model number: 'ABC-123'
Press Ctrl+C to stop...

[/dev/ttyUSB0] Programming device AC:0B:FB:6C:F5:34...
[/dev/ttyUSB0] ✓ SUCCESS: AC:0B:FB:6C:F5:34 programmed with 'ABC-123'

[/dev/ttyUSB1] Programming device AA:BB:CC:DD:EE:FF...
[/dev/ttyUSB1] ✓ SUCCESS: AA:BB:CC:DD:EE:FF programmed with 'ABC-123'

^C

Shutting down...
Waiting for active programming operations to complete...

============================================================
Successfully programmed 2 device(s):
============================================================
  AA:BB:CC:DD:EE:FF
  AC:0B:FB:6C:F5:34
============================================================

Shutdown complete.
```

Debug mode (verbose output):
```
Monitoring for ESP32 devices to program with model number: 'ABC-123'
DEBUG MODE ENABLED - Verbose output will be shown
Press Ctrl+C to stop...

DEBUG: Current ports detected: {'/dev/ttyUSB0'}
[/dev/ttyUSB0] New device detected!
[/dev/ttyUSB0] Attempting to program device...
[/dev/ttyUSB0] DEBUG: Opening serial port at 115200 baud
[/dev/ttyUSB0] DEBUG: Serial port opened successfully
[/dev/ttyUSB0] Waiting for console prompt...
[/dev/ttyUSB0] Getting device MAC address...
[/dev/ttyUSB0] Device MAC: AC:0B:FB:6C:F5:34
[/dev/ttyUSB0] Programming device AC:0B:FB:6C:F5:34...
[/dev/ttyUSB0] Model number set successfully
[/dev/ttyUSB0] Verifying with 'model get' command...
[/dev/ttyUSB0] ✓ SUCCESS: AC:0B:FB:6C:F5:34 programmed with 'ABC-123'
```

## Troubleshooting

### No prompt detected
- **Reset the ESP32 device**: Press the reset button or power cycle the device
- Ensure the ESP32 is running the console application
- Check that the baudrate matches (default: 115200)
- Verify the USB cable supports data transfer
- Make sure no other program (like idf.py monitor) is using the serial port

### Permission denied on Linux
- Add your user to the dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
- Log out and back in for changes to take effect

### Device not detected
- Check USB connection
- Verify device appears in system (Linux: `ls /dev/ttyUSB*`, macOS: `ls /dev/tty.*`)
- Try increasing the polling interval with `-i` option

## Stopping the Program

Press `Ctrl+C` to stop monitoring. The program will wait for any active programming operations to complete before shutting down.

