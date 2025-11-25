# Simple CLI Component

A lightweight C++ wrapper around ESP-IDF's console component and linenoise library, providing an easy-to-use command-line interface for ESP32 applications.

## Features

- **Multiple Interface Support**: UART, USB Serial JTAG, or automatic USB with UART fallback
- **Simple Configuration**: Minimal boilerplate to get a CLI running
- **Type-Safe Command Registration**: Uses C++ `std::array` for compile-time safety
- **Pre-configured Defaults**: Sensible defaults for buffer sizes, line endings, and console behavior
- **Background Operation**: Runs in a dedicated FreeRTOS task
- **Built-in Help**: Automatically registers help command for easy command discovery

## Requirements

- ESP-IDF >= 4.1.0
- C++17 or later
- espressif/esp_linenoise component (automatically managed)

## Installation

### As a Local Component

Add the `simple_cli` directory to your project's `components` folder or reference it using `EXTRA_COMPONENT_DIRS` in your project's CMakeLists.txt:

```cmake
set(EXTRA_COMPONENT_DIRS "../shared_components/simple_cli")
```

### Component Dependencies

The component will automatically pull in the required `espressif/esp_linenoise` dependency via the IDF Component Manager.

## Usage

### Basic Setup

```cpp
#include "simple_cli.h"
#include <array>

// Define your command handler
static int restart(int argc, char** argv) {
  ESP_LOGI("CMD", "Restarting system...");
  esp_restart();
  return 0;
}

// Register commands using std::array
constexpr std::array<esp_console_cmd_t, 1> commands = {{{
    .command = "restart",
    .help = "Restart the system",
    .hint = NULL,
    .func = &restart,
    .argtable = NULL,
    .func_w_context = NULL,
    .context = NULL,
}}};

extern "C" void app_main(void) {
  // Create CLI instance with prompt and interface type
  SimpleCLI cli("my-app> ", SimpleCLIInterface::USBSerialJTAG);
  
  // Register your commands
  cli.register_commands(commands);
  
  // Start the CLI (runs in background task)
  cli.start();
  
  // Your main application logic continues here
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
```

### Interface Options

The available interface options depend on your project's sdkconfig settings:

#### USB Serial JTAG Only
```cpp
SimpleCLI cli("prompt> ", SimpleCLIInterface::USBSerialJTAG);
```

#### UART Only
```cpp
SimpleCLI cli("prompt> ", SimpleCLIInterface::UART);
```

#### USB with UART Fallback
```cpp
// Automatically uses USB if connected, falls back to UART otherwise
SimpleCLI cli("prompt> ", SimpleCLIInterface::USBFallbackToUART);
```

### Registering Multiple Commands

```cpp
static int cmd1(int argc, char** argv) { /* ... */ }
static int cmd2(int argc, char** argv) { /* ... */ }
static int cmd3(int argc, char** argv) { /* ... */ }

constexpr std::array<esp_console_cmd_t, 3> commands = {{{
    { .command = "cmd1", .help = "First command", .func = &cmd1 },
    { .command = "cmd2", .help = "Second command", .func = &cmd2 },
    { .command = "cmd3", .help = "Third command", .func = &cmd3 }
}}};

SimpleCLI cli("app> ", SimpleCLIInterface::UART);
cli.register_commands(commands);
cli.start();
```

### Registering Individual Commands

```cpp
esp_console_cmd_t single_cmd = {
    .command = "status",
    .help = "Show system status",
    .func = &status_handler
};

cli.register_command(single_cmd);
```

### Using Argument Tables

For commands with arguments, use ESP-IDF's argtable3 library:

```cpp
#include "argtable3/argtable3.h"

static struct {
  struct arg_int* timeout;
  struct arg_end* end;
} connect_args;

static int connect_handler(int argc, char** argv) {
  int nerrors = arg_parse(argc, argv, (void**)&connect_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, connect_args.end, argv[0]);
    return 1;
  }
  
  int timeout = connect_args.timeout->ival[0];
  printf("Connecting with timeout: %d\n", timeout);
  return 0;
}

void register_connect_command(SimpleCLI& cli) {
  connect_args.timeout = arg_int1(NULL, "timeout", "<t>", "Connection timeout in seconds");
  connect_args.end = arg_end(2);
  
  esp_console_cmd_t cmd = {
    .command = "connect",
    .help = "Connect to network",
    .hint = "--timeout <seconds>",
    .func = &connect_handler,
    .argtable = &connect_args
  };
  
  cli.register_command(cmd);
}
```

## Architecture

The component provides a thin wrapper that:

1. **Initializes the ESP Console**: Configures the console singleton with sensible defaults (256-byte command line, 8 arguments max)
2. **Sets up the I/O Interface**: Configures UART or USB Serial JTAG drivers and VFS integration
3. **Configures Linenoise**: Sets up the line editing library with the chosen input/output file descriptors
4. **Runs the REPL**: Executes a read-eval-print loop in a dedicated FreeRTOS task (4KB stack, priority: idle + 5)


## Example Project

See the [simple-cli example](../../simple-cli/) for a complete working implementation.

## API Reference

### Class: `SimpleCLI`

#### Constructor
```cpp
SimpleCLI(const std::string prompt, SimpleCLIInterface interface)
```
- `prompt`: The command prompt string (e.g., "my-app> ")
- `interface`: The I/O interface to use

#### Methods

**`void start()`**
- Initializes the console and starts the CLI task
- Must be called after registering commands
- Non-blocking (CLI runs in background)

**`void stop()`**
- Stops the CLI task
- Note: Currently sets running flag but doesn't clean up resources

**`template <size_t N> void register_commands(const std::array<esp_console_cmd_t, N>& commands)`**
- Register multiple commands from a std::array
- Template parameter N is automatically deduced
- Type-safe at compile time

**`void register_command(const esp_console_cmd_t& command)`**
- Register a single command
- Can be called before or after `start()`

### Enum: `SimpleCLIInterface`

Available values depend on sdkconfig (console channel configuration):
- `UART`: Use UART0 for CLI
- `USBSerialJTAG`: Use USB Serial JTAG for CLI
- `USBFallbackToUART`: Try USB first, fall back to UART if not connected

## Troubleshooting

### No Interface Available Error
If you get a compile error about no console interface being defined, ensure your sdkconfig has at least one of:
- `CONFIG_ESP_CONSOLE_UART_DEFAULT`
- `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`
- `CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG`

### Commands Not Working
- Ensure `cli.start()` is called after registering all commands
- Check that command handlers have the correct signature: `int func(int argc, char** argv)`
- Use the built-in `help` command to verify commands are registered

### USB Serial JTAG Not Showing
- Verify USB cable supports data transfer (not just power)
- Check that `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` is enabled in sdkconfig
- Use `USBFallbackToUART` interface for development flexibility
