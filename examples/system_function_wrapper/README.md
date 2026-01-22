# Wrapping/Replacing Native IDF Functions 

This example demonstrates how to wrap or completely replace ESP-IDF functions using the GNU linker's `--wrap` feature. This powerful technique allows you to customize IDF behavior without modifying the framework's source code, keeping your customizations clean and maintainable.

**Full Article:** [Redefine IDF Functions](https://productionesp32.com/posts/redefine-idf-functions/)

## What This Example Shows

This project replaces `esp_restart()` When any code calls `esp_restart()`, the replacement executes instead.

## Key Changes Required to Enable Function Wrapping

### 1. Add Linker Flag in CMakeLists.txt

In your project's top-level `CMakeLists.txt`, add the linker wrap directive **after** the `project()` command:

```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_project)

# Tell the linker to wrap esp_restart
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=esp_restart")
```

The format is always `-Wl,--wrap=function_name` where `function_name` is the exact name of the function you want to wrap.

### 2. Implement the Wrapper Function

Create your wrapper implementation with the `__wrap_` prefix. Two key parts are required:

```c
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "WRAPPER";

extern "C" {
// 1. Declare the original function
void __real_esp_restart(void);

// 2. Implement the wrapper
void __wrap_esp_restart(void)
{
    ESP_LOGW(TAG, "System restart requested - performing cleanup...");
    
    // Add any custom pre-restart logic here
    // For example: save state, close connections, etc.
    
    // Call the original implementation
    __real_esp_restart();
}
} // extern "C"
```

**Important Notes:**
- `__real_function_name` - Declaration to access the original implementation
- `__wrap_function_name` - Your wrapper that gets called instead
- The wrapper must have the **exact same signature** as the original function
- For C++ files, use `extern "C"` for both declarations to prevent name mangling

### 3. Build and Test

Do a clean build to ensure the linker flags are applied:

```bash
idf.py fullclean
idf.py flash monitor
```

## Use Cases

- **Debug Logging** - Track when and how functions are called without modifying IDF
- **Testing and Mocking** - Replace hardware-dependent functions for simulation
- **Performance Optimization** - Substitute with custom optimized implementations
- **Behavioral Modification** - Add validation, caching, or other functionality
- **Temporary Workarounds** - Apply quick fixes to IDF bugs without forking

## Wrapping Multiple Functions

You can wrap multiple functions by adding multiple linker flags:

```cmake
target_link_libraries(${COMPONENT_LIB} INTERFACE 
    "-Wl,--wrap=esp_restart"
    "-Wl,--wrap=esp_deep_sleep_start"
    "-Wl,--wrap=nvs_flash_init"
)
```

Then implement a wrapper for each one in your source files.

## Completely Replacing a Function

To completely replace a function without calling the original, simply omit the call to `__real_function_name`:

```c
void __wrap_esp_restart(void)
{
    ESP_LOGI(TAG, "Restart blocked for testing");
    // Original function is NOT called
}
```
