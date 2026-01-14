# MCP Server for ESP32

A lightweight, transport-agnostic MCP (Model Context Protocol) server implementation for ESP32 using ESP-IDF. This library provides a simple declarative API for creating tools that can be accessed over HTTP (with future support for UART and WebSocket).

## Features

- ✅ **Transport Abstraction** - Easy to swap between HTTP, UART, or WebSocket at compile time
- ✅ **Declarative API** - Simple tool registration with minimal boilerplate
- ✅ **MCP Protocol Compliant** - Full JSON-RPC 2.0 implementation with initialize handshake
- ✅ **VSCode Integration** - Works seamlessly with VSCode's MCP client
- ✅ **ESP-IDF Native** - Uses built-in components (cJSON, esp_http_server)
- ✅ **CORS Support** - Ready for web-based clients
- ✅ **Extensible** - Easy to add new tools and transports
- ✅ **Memory Efficient** - Minimal allocations with proper cleanup

## Architecture

```
┌─────────────────────────────────────────┐
│         User Application Layer          │
│  (main.cpp - tool registration)         │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│         MCP Server Core Layer           │
│  - Tool registry & management           │
│  - Request/response orchestration       │
│  - Protocol handling (JSON-RPC 2.0)     │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│      Transport Abstraction Layer        │
│  Interface: MCPTransport (virtual)      │
└─────────────────────────────────────────┘
                    ↓
    ┌───────────────┴───────────────┐
    ↓                               ↓
┌────────────────┐         ┌────────────────┐
│ HTTP Transport │         │ UART Transport │
│   (Available)  │         │    (Future)    │
└────────────────┘         └────────────────┘
```

## Quick Start

### 1. Basic Usage

```c
#include "mcp_lib/include/mcp_server.h"
#include "wifi_connect.h"

// Define a tool handler
static mcp_tool_result_t hello_world_handler(const mcp_tool_args_t* args) {
    return mcp_tool_result_success("Hello from ESP32!");
}

// Declarative tool definition
static const mcp_tool_definition_t HELLO_WORLD_TOOL = {
    .name = "hello_world",
    .description = "Returns a friendly greeting",
    .handler = hello_world_handler,
};

void app_main(void) {
    // Connect to WiFi
    connect_to_wifi();
    while (!is_wifi_connected()) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Create MCP server with HTTP transport
    mcp_server_t* server = mcp_server_create(MCP_TRANSPORT_HTTP);
    
    // Register tool
    mcp_server_register_tool(server, &HELLO_WORLD_TOOL);
    
    // Start server on port 3000
    mcp_server_start(server, 3000);
}
```

### 2. Advanced Tool with Arguments and Parameter Schema

Tools can define parameter schemas that are exposed to MCP clients, enabling proper validation and IDE support.

```c
#include "mcp_schema.h"

static mcp_tool_result_t echo_handler(const mcp_tool_args_t* args) {
    // Get argument from JSON
    const char* message = mcp_tool_args_get_string(args, "message", "");
    
    if (strlen(message) == 0) {
        return mcp_tool_result_error("Missing 'message' parameter");
    }
    
    // Do some processing
    char response[256];
    snprintf(response, sizeof(response), "Echo: %s", message);
    
    return mcp_tool_result_success(response);
}

// Define parameter schema
static const mcp_param_schema_t ECHO_PARAMS[] = {
    MCP_PARAM_STRING_REQUIRED("message", "The message to echo back")
};

static const mcp_tool_definition_t ECHO_TOOL = {
    .name = "echo",
    .description = "Echoes back the provided message",
    .handler = echo_handler,
    .parameters = ECHO_PARAMS,
    .parameter_count = sizeof(ECHO_PARAMS) / sizeof(ECHO_PARAMS[0])
};
```

### 3. Tool with Numeric Parameters and Validation

```c
static mcp_tool_result_t set_thermostat_handler(const mcp_tool_args_t* args) {
    double temperature = mcp_tool_args_get_double(args, "temperature", -999.0);
    
    // Validate temperature
    if (temperature == -999.0 || temperature < 40.0 || temperature > 90.0) {
        return mcp_tool_result_error("Invalid temperature (must be between 40 and 90 degrees)");
    }
    
    // Set the thermostat
    set_target_temperature(temperature);
    
    char result[64];
    snprintf(result, sizeof(result), "{\"setpoint\": %.1f, \"status\": \"success\"}", temperature);
    return mcp_tool_result_success(result);
}

// Define parameter schema with constraints
static const mcp_param_schema_t THERMOSTAT_PARAMS[] = {
    MCP_PARAM_NUMBER_REQUIRED("temperature", 
                              "Target temperature in Fahrenheit", 
                              40.0,  // minimum
                              90.0)  // maximum
};

static const mcp_tool_definition_t THERMOSTAT_TOOL = {
    .name = "set_thermostat",
    .description = "Sets the thermostat setpoint temperature in Fahrenheit",
    .handler = set_thermostat_handler,
    .parameters = THERMOSTAT_PARAMS,
    .parameter_count = sizeof(THERMOSTAT_PARAMS) / sizeof(THERMOSTAT_PARAMS[0])
};
```

### 4. Available Parameter Schema Macros

The MCP library provides convenient macros for defining parameter schemas:

```c
// String parameters
MCP_PARAM_STRING_REQUIRED("name", "Description")
MCP_PARAM_STRING("name", "Description")  // Optional

// Numeric parameters (double)
MCP_PARAM_NUMBER_REQUIRED("value", "Description", min, max)
MCP_PARAM_NUMBER("value", "Description", min, max)  // Optional

// Integer parameters
MCP_PARAM_INTEGER_REQUIRED("count", "Description", min, max)
MCP_PARAM_INTEGER("count", "Description", min, max)  // Optional

// Boolean parameters
MCP_PARAM_BOOLEAN_REQUIRED("enabled", "Description")
MCP_PARAM_BOOLEAN("enabled", "Description")  // Optional
```

Example with multiple parameters:

```c
static const mcp_param_schema_t LED_CONTROL_PARAMS[] = {
    MCP_PARAM_INTEGER_REQUIRED("led_number", "LED index (0-7)", 0, 7),
    MCP_PARAM_BOOLEAN_REQUIRED("state", "Turn LED on (true) or off (false)"),
    MCP_PARAM_INTEGER("brightness", "Brightness level (0-255)", 0, 255)  // Optional
};
```

### 5. Imperative API (Alternative)

For simple cases or dynamic tool registration, you can use the imperative API:

```c
// Create server
mcp_server_t* server = mcp_server_create(MCP_TRANSPORT_HTTP);

// Register tool using imperative API (no parameter schema support)
mcp_server_add_tool(server, "hello", "Says hello", hello_handler);

// Register multiple tools with schemas (declarative API recommended)
const mcp_tool_definition_t* tools[] = {
    &HELLO_WORLD_TOOL,
    &ECHO_TOOL,
    &THERMOSTAT_TOOL,
};
mcp_server_register_tools(server, tools, 3);
```

**Note:** The imperative `mcp_server_add_tool()` API does not support parameter schemas. Use the declarative API with `mcp_tool_definition_t` for tools that require parameters.

## API Reference

### Server Management

```c
// Create server with specific transport
mcp_server_t* mcp_server_create(mcp_transport_type_t transport_type);

// Destroy server and free resources
void mcp_server_destroy(mcp_server_t* server);

// Start server on specified port
esp_err_t mcp_server_start(mcp_server_t* server, uint16_t port);

// Stop server
esp_err_t mcp_server_stop(mcp_server_t* server);
```

### Tool Registration

```c
// Declarative API (recommended)
esp_err_t mcp_server_register_tool(mcp_server_t* server, 
                                    const mcp_tool_definition_t* tool);

esp_err_t mcp_server_register_tools(mcp_server_t* server, 
                                     const mcp_tool_definition_t* tools[], 
                                     size_t count);

// Imperative API
esp_err_t mcp_server_add_tool(mcp_server_t* server, 
                              const char* name, 
                              const char* description,
                              mcp_tool_handler_t handler);
```

### Tool Helpers

```c
// Get arguments from JSON
const char* mcp_tool_args_get_string(const mcp_tool_args_t* args, 
                                     const char* key, 
                                     const char* default_val);
int mcp_tool_args_get_int(const mcp_tool_args_t* args, 
                          const char* key, 
                          int default_val);
bool mcp_tool_args_get_bool(const mcp_tool_args_t* args, 
                            const char* key, 
                            bool default_val);
double mcp_tool_args_get_double(const mcp_tool_args_t* args,
                                const char* key,
                                double default_val);

// Create results
mcp_tool_result_t mcp_tool_result_success(const char* content);
mcp_tool_result_t mcp_tool_result_error(const char* error_message);
void mcp_tool_result_free(mcp_tool_result_t* result);
```

### Parameter Schema Definition

```c
// Define parameter schemas for tools
typedef struct {
  const char* name;
  mcp_param_type_t type;
  const char* description;
  bool required;
  double minimum;    // For NUMBER/INTEGER types
  double maximum;    // For NUMBER/INTEGER types
  // ... other fields for string constraints, enums, etc.
} mcp_param_schema_t;

// Helper macros (see mcp_schema.h)
MCP_PARAM_STRING_REQUIRED(name, description)
MCP_PARAM_NUMBER_REQUIRED(name, description, min, max)
MCP_PARAM_INTEGER_REQUIRED(name, description, min, max)
MCP_PARAM_BOOLEAN_REQUIRED(name, description)
// ... and optional variants
```

## MCP Protocol

The server implements the MCP JSON-RPC 2.0 specification.

### List Tools

**Request:**
```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/list"
}
```

**Response:**
```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
        "tools": [
            {
                "name": "hello_world",
                "description": "Returns a friendly greeting from the ESP32",
                "inputSchema": {
                    "type": "object",
                    "properties": {}
                }
            },
            {
                "name": "set_thermostat",
                "description": "Sets the thermostat setpoint temperature",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "temperature": {
                            "type": "number",
                            "description": "Target temperature in Fahrenheit",
                            "minimum": 40,
                            "maximum": 90
                        }
                    },
                    "required": ["temperature"]
                }
            }
        ]
    }
}
```

### Call Tool

**Request (no parameters):**
```json
{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "tools/call",
    "params": {
        "name": "hello_world",
        "arguments": {}
    }
}
```

**Request (with parameters):**
```json
{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "tools/call",
    "params": {
        "name": "set_thermostat",
        "arguments": {
            "temperature": 72.0
        }
    }
}
```

**Response (success):**
```json
{
    "jsonrpc": "2.0",
    "id": 2,
    "result": {
        "content": [
            {
                "type": "text",
                "text": "Hello from ESP32 MCP Server!"
            }
        ]
    }
}
```

**Response (error):**
```json
{
    "jsonrpc": "2.0",
    "id": 3,
    "result": {
        "error": "Invalid temperature value (must be between 40 and 90 degrees)"
    }
}
```

## Testing

### Using curl

```bash
# List available tools
curl -X POST http://esp32.local:3000 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "tools/list"
  }'

# Call hello_world tool (no parameters)
curl -X POST http://esp32.local:3000 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 2,
    "method": "tools/call",
    "params": {
      "name": "hello_world",
      "arguments": {}
    }
  }'

# Call set_thermostat tool (with parameters)
curl -X POST http://esp32.local:3000 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "id": 3,
    "method": "tools/call",
    "params": {
      "name": "set_thermostat",
      "arguments": {
        "temperature": 72.0
      }
    }
  }'
```

## Configuration

### WiFi Credentials

Configure WiFi credentials in `menuconfig`:

```bash
idf.py menuconfig
# Navigate to: Component config → WiFi Configuration
```

Or set via environment variables before connecting.

### HTTP Server Port

Default port is 3000. Change in your application:

```c
mcp_server_start(server, 8080);  // Use port 8080
```

## Component Structure

```
mcp_server/                             # MCP server component
├── include/
│   ├── mcp_server.h                    # Main server API
│   ├── mcp_tool.h                      # Tool definition & result
│   ├── mcp_transport.h                 # Transport interface
│   ├── mcp_protocol.h                  # JSON-RPC protocol
│   ├── mcp_schema.h                    # Parameter schema definitions
│   └── mcp_types.h                     # Common types & enums
├── src/
│   ├── mcp_server.c                    # Server implementation
│   ├── mcp_tool.c                      # Tool management
│   ├── mcp_protocol.c                  # JSON-RPC protocol
│   ├── mcp_transport.c                 # Transport base
│   └── mcp_schema.c                    # Schema generation
├── transports/
│   ├── mcp_transport_http.h
│   └── mcp_transport_http.c            # HTTP transport implementation
└── CMakeLists.txt
```

## Future Enhancements

- [ ] UART transport implementation
- [ ] WebSocket transport
- [ ] Tool input schema validation
- [Advanced Usage

## License

This example is in the Public Domain (or CC0 licensed, at your option).
