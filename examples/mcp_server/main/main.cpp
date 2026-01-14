#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mcp_schema.h"
#include "mcp_server.h"
#include "wifi_connect.h"

static const char* TAG = "main";
static double thermostat_setpoint = 72.0;  // Default setpoint in degrees

/**
 * @brief Hello World tool handler
 *
 * This is a simple example tool that returns a greeting message.
 */
static mcp_tool_result_t hello_world_handler(const mcp_tool_args_t* args) {
  ESP_LOGI(TAG, "Hello World tool called");
  return mcp_tool_result_success("Hello from ESP32 MCP Server!");
}

/**
 * @brief Get Temperature tool handler
 *
 * Returns the current temperature reading.
 * Currently returns a random value between 40 and 80 degrees.
 * TODO: Replace with actual sensor reading.
 */
static mcp_tool_result_t get_temperature_handler(const mcp_tool_args_t* args) {
  ESP_LOGI(TAG, "Get Temperature tool called");

  // Generate random temperature between 40 and 80 degrees
  float temperature = 40.0f + ((float)(rand() % 41));

  char result[64];
  snprintf(result, sizeof(result), "{\"temperature\": %.1f, \"unit\": \"F\"}", temperature);

  return mcp_tool_result_success(result);
}

/**
 * @brief Set Thermostat tool handler
 *
 * Sets the thermostat setpoint temperature.
 * Currently stores in a global variable.
 * TODO: Integrate with actual thermostat control.
 */
static mcp_tool_result_t set_thermostat_handler(const mcp_tool_args_t* args) {
  ESP_LOGI(TAG, "Set Thermostat tool called");

  // Get the temperature value from arguments using helper function
  double new_setpoint = mcp_tool_args_get_double(args, "temperature", -999.0);

  // Validate the temperature is within a reasonable range
  if (new_setpoint == -999.0 || new_setpoint < 40.0 || new_setpoint > 90.0) {
    return mcp_tool_result_error("Invalid temperature value (must be between 40 and 90 degrees)");
  }

  thermostat_setpoint = new_setpoint;

  char result[64];
  snprintf(result, sizeof(result), "{\"setpoint\": %.1f, \"status\": \"success\"}", thermostat_setpoint);

  ESP_LOGI(TAG, "Thermostat setpoint updated to %.1f", thermostat_setpoint);
  return mcp_tool_result_success(result);
}

/**
 * @brief Declarative tool definition for hello_world
 */
static const mcp_tool_definition_t HELLO_WORLD_TOOL = {
    .name = "hello_world",
    .description = "Returns a friendly greeting from the ESP32",
    .handler = hello_world_handler,
    .parameters = NULL,
    .parameter_count = 0,
};

/**
 * @brief Declarative tool definition for get_temperature
 */
static const mcp_tool_definition_t GET_TEMPERATURE_TOOL = {
    .name = "get_temperature",
    .description = "Gets the current temperature reading in Fahrenheit",
    .handler = get_temperature_handler,
    .parameters = NULL,
    .parameter_count = 0,
};

/**
 * @brief Parameter schema for set_thermostat tool
 */
static const mcp_param_schema_t SET_THERMOSTAT_PARAMS[] = {
    MCP_PARAM_NUMBER_REQUIRED("temperature", "Target temperature in Fahrenheit (must be between 40 and 90)", 40.0,
                              90.0),
};

/**
 * @brief Declarative tool definition for set_thermostat
 */
static const mcp_tool_definition_t SET_THERMOSTAT_TOOL = {
    .name = "set_thermostat",
    .description = "Sets the thermostat setpoint temperature in Fahrenheit",
    .handler = set_thermostat_handler,
    .parameters = SET_THERMOSTAT_PARAMS,
    .parameter_count = sizeof(SET_THERMOSTAT_PARAMS) / sizeof(SET_THERMOSTAT_PARAMS[0]),
};

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "Starting MCP Server example");

  // Connect to WiFi
  ESP_LOGI(TAG, "Connecting to WiFi...");
  connect_to_wifi();

  // Wait for WiFi connection
  while (!is_wifi_connected()) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  ESP_LOGI(TAG, "WiFi connected!");

  // Create MCP server with HTTP transport
  mcp_server_t* server = mcp_server_create(MCP_TRANSPORT_HTTP);
  if (!server) {
    ESP_LOGE(TAG, "Failed to create MCP server");
    return;
  }

  // Register all tools using declarative API
  const mcp_tool_definition_t* tools[] = {
      &HELLO_WORLD_TOOL,
      &GET_TEMPERATURE_TOOL,
      &SET_THERMOSTAT_TOOL,
  };

  esp_err_t ret = mcp_server_register_tools(server, tools, sizeof(tools) / sizeof(tools[0]));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register tools");
    mcp_server_destroy(server);
    return;
  }

  // Start the server on port 3000
  ret = mcp_server_start(server, 3000);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start MCP server");
    mcp_server_destroy(server);
    return;
  }

  ESP_LOGI(TAG, "MCP Server running on port 3000");
  ESP_LOGI(TAG, "Registered tools: %zu", mcp_server_get_tool_count(server));
  ESP_LOGI(TAG, "Ready to accept requests!");

  // Keep running
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}