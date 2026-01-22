# Production ESP32 Playground

This repo is a research playground containing code samples for the ESP32.
It was built to allow for quickly spinning up new projects with the minimal amount of boilerplate code. 

## Example Projects

- [**dtf_simple**](examples/dtf_simple/README.md) - Integration example with Deploy the Fleet OTA service
- [**factory_floor_cli**](examples/factory_floor_cli/README.md) - Factory device provisioning with ESP32 console and Python script
- [**including_local_components**](examples/including_local_components/) - Demonstrates how to include local custom components
- [**mcp_server**](examples/mcp_server/) - Model Context Protocol (MCP) server running on ESP32 with HTTP transport
- [**minimal_build**](examples/minimal_build/README.md) - Template project using minimal build settings to reduce compile time
- [**qemu_with_debug**](examples/qemu_with_debug/README.md) - Step debugging ESP32 applications using QEMU emulator
- [**qemu_with_internet**](examples/qemu_with_internet/README.md) - Internet access in QEMU via Ethernet with DNS and HTTPS examples
- [**simple-cli**](examples/simple-cli/README.md) - Basic example using the simple_cli custom component
- [**system_function_wrapper**](examples/system_function_wrapper/) - Demonstrates how to wrap or replace native ESP-IDF functions

## Shared Components

Reusable ESP-IDF components located in [examples/shared_components](examples/shared_components/):

- [**mcp_server**](examples/shared_components/mcp_server/README.md) - Lightweight Model Context Protocol server library with transport abstraction
- [**qemu_internet**](examples/shared_components/qemu_internet/README.md) - Enables internet access for ESP32 projects running in QEMU
- [**simple_cli**](examples/shared_components/simple_cli/README.md) - C++ wrapper for ESP-IDF console with linenoise support
- [**wifi_connect**](examples/shared_components/wifi_connect/) - Simple WiFi connection helper component

## Custom Shell Tools

This repository includes custom bash functions and tools defined in [.devcontainer/.bash_aliases](.devcontainer/.bash_aliases):

- **`setchip`** - Interactive menu to select ESP32 target chip (esp32, esp32s3, esp32c3, etc.) and configure GDB paths
- **`debugthis`** - Configure VS Code debugger settings to debug the project. Run this from the root of the project you want to step debug.
- **`np`** - Create a new ESP32 project from the minimal_build template with automatic setup