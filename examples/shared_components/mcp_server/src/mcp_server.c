#include "mcp_server.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "mcp_protocol.h"
#include "mcp_schema.h"
#include "mcp_transport_http.h"

static const char* TAG = "mcp_server";

#define MAX_TOOLS 32

/**
 * @brief Tool registry entry (internal)
 */
typedef struct {
  char* name;
  char* description;
  mcp_tool_handler_t handler;
  const mcp_param_schema_t* parameters;  // Pointer to parameter schema array
  size_t parameter_count;                // Number of parameters
} tool_entry_t;

/**
 * @brief MCP Server implementation
 */
struct mcp_server {
  mcp_transport_t* transport;
  tool_entry_t tools[MAX_TOOLS];
  size_t tool_count;
  bool is_running;
  bool is_initialized;     // Track initialization state
  char* protocol_version;  // Client protocol version
  char* client_name;       // Client name for logging
};

// Forward declarations
static void handle_request(const char* request, void* context);
static void handle_initialize(mcp_server_t* server, cJSON* params, int request_id);
static void handle_initialized(mcp_server_t* server);
static void handle_list_tools(mcp_server_t* server, int request_id);
static void handle_call_tool(mcp_server_t* server, const char* tool_name, cJSON* args, int request_id);
static tool_entry_t* find_tool(mcp_server_t* server, const char* name);

mcp_server_t* mcp_server_create(mcp_transport_type_t transport_type) {
  mcp_server_t* server = calloc(1, sizeof(mcp_server_t));
  if (!server) {
    ESP_LOGE(TAG, "Failed to allocate server");
    return NULL;
  }

  // Create transport based on type
  switch (transport_type) {
    case MCP_TRANSPORT_HTTP:
      server->transport = mcp_transport_http_create();
      break;
    case MCP_TRANSPORT_UART:
    case MCP_TRANSPORT_WEBSOCKET:
      ESP_LOGE(TAG, "Transport type not yet implemented");
      free(server);
      return NULL;
    default:
      ESP_LOGE(TAG, "Unknown transport type");
      free(server);
      return NULL;
  }

  if (!server->transport) {
    ESP_LOGE(TAG, "Failed to create transport");
    free(server);
    return NULL;
  }

  // Set request handler
  mcp_transport_set_request_handler(server->transport, handle_request, server);

  // Initialize state
  server->is_initialized = false;
  server->protocol_version = NULL;
  server->client_name = NULL;

  ESP_LOGI(TAG, "MCP server created");
  return server;
}

void mcp_server_destroy(mcp_server_t* server) {
  if (!server) {
    return;
  }

  // Stop if running
  if (server->is_running) {
    mcp_server_stop(server);
  }

  // Free tools
  for (size_t i = 0; i < server->tool_count; i++) {
    if (server->tools[i].name) {
      free(server->tools[i].name);
    }
    if (server->tools[i].description) {
      free(server->tools[i].description);
    }
  }

  // Free initialization data
  if (server->protocol_version) {
    free(server->protocol_version);
  }
  if (server->client_name) {
    free(server->client_name);
  }

  // Destroy transport
  if (server->transport && server->transport->vtable && server->transport->vtable->destroy) {
    server->transport->vtable->destroy(server->transport);
  }

  free(server);
  ESP_LOGI(TAG, "MCP server destroyed");
}

esp_err_t mcp_server_add_tool(mcp_server_t* server, const char* name, const char* description,
                              mcp_tool_handler_t handler) {
  if (!server || !name || !handler) {
    return ESP_ERR_INVALID_ARG;
  }

  mcp_tool_definition_t tool = {.name = name, .description = description, .handler = handler};

  return mcp_server_register_tool(server, &tool);
}

esp_err_t mcp_server_register_tool(mcp_server_t* server, const mcp_tool_definition_t* tool) {
  if (!server || !tool || !tool->name || !tool->handler) {
    return ESP_ERR_INVALID_ARG;
  }

  // Check if tool already exists
  if (find_tool(server, tool->name)) {
    ESP_LOGE(TAG, "Tool '%s' already registered", tool->name);
    return ESP_ERR_INVALID_STATE;
  }

  // Check capacity
  if (server->tool_count >= MAX_TOOLS) {
    ESP_LOGE(TAG, "Maximum number of tools (%d) reached", MAX_TOOLS);
    return ESP_ERR_NO_MEM;
  }

  // Add tool
  tool_entry_t* entry = &server->tools[server->tool_count];
  entry->name = strdup(tool->name);
  entry->description = tool->description ? strdup(tool->description) : NULL;
  entry->handler = tool->handler;
  entry->parameters = tool->parameters;  // Store pointer to parameter schema
  entry->parameter_count = tool->parameter_count;

  if (!entry->name) {
    ESP_LOGE(TAG, "Failed to allocate tool name");
    return ESP_ERR_NO_MEM;
  }

  server->tool_count++;
  ESP_LOGI(TAG, "Registered tool: %s", tool->name);

  return ESP_OK;
}

esp_err_t mcp_server_register_tools(mcp_server_t* server, const mcp_tool_definition_t* tools[], size_t count) {
  if (!server || !tools) {
    return ESP_ERR_INVALID_ARG;
  }

  for (size_t i = 0; i < count; i++) {
    esp_err_t ret = mcp_server_register_tool(server, tools[i]);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to register tool %zu", i);
      return ret;
    }
  }

  return ESP_OK;
}

esp_err_t mcp_server_start(mcp_server_t* server, uint16_t port) {
  if (!server) {
    return ESP_ERR_INVALID_ARG;
  }

  if (server->is_running) {
    ESP_LOGW(TAG, "Server already running");
    return ESP_ERR_INVALID_STATE;
  }

  // Initialize transport
  esp_err_t ret = server->transport->vtable->init(server->transport, port);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize transport");
    return ret;
  }

  // Start transport
  ret = server->transport->vtable->start(server->transport);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start transport");
    return ret;
  }

  server->is_running = true;
  ESP_LOGI(TAG, "MCP server started with %zu tools", server->tool_count);

  return ESP_OK;
}

esp_err_t mcp_server_stop(mcp_server_t* server) {
  if (!server) {
    return ESP_ERR_INVALID_ARG;
  }

  if (!server->is_running) {
    ESP_LOGW(TAG, "Server not running");
    return ESP_OK;
  }

  esp_err_t ret = server->transport->vtable->stop(server->transport);
  server->is_running = false;

  ESP_LOGI(TAG, "MCP server stopped");
  return ret;
}

size_t mcp_server_get_tool_count(const mcp_server_t* server) { return server ? server->tool_count : 0; }

bool mcp_server_has_tool(const mcp_server_t* server, const char* name) {
  return find_tool((mcp_server_t*)server, name) != NULL;
}

// Internal helpers

static void handle_initialize(mcp_server_t* server, cJSON* params, int request_id) {
  ESP_LOGI(TAG, "Handling initialize request");

  // Parse client info
  if (params) {
    cJSON* protocol_version = cJSON_GetObjectItem(params, "protocolVersion");
    if (protocol_version && cJSON_IsString(protocol_version)) {
      server->protocol_version = strdup(protocol_version->valuestring);
      ESP_LOGI(TAG, "Client protocol version: %s", server->protocol_version);
    }

    cJSON* client_info = cJSON_GetObjectItem(params, "clientInfo");
    if (client_info) {
      cJSON* name = cJSON_GetObjectItem(client_info, "name");
      if (name && cJSON_IsString(name)) {
        server->client_name = strdup(name->valuestring);
        ESP_LOGI(TAG, "Client name: %s", server->client_name);
      }
    }
  }

  // Mark as initialized (but wait for initialized notification before accepting tool calls)
  // Some clients send initialized notification, some don't

  // Build capabilities response
  cJSON* result = cJSON_CreateObject();
  if (!result) {
    char* error_resp = mcp_protocol_create_error(request_id, -32603, "Internal error");
    if (error_resp) {
      server->transport->vtable->send_response(server->transport, error_resp);
      free(error_resp);
    }
    return;
  }

  // Server capabilities
  cJSON* capabilities = cJSON_CreateObject();
  cJSON* tools_cap = cJSON_CreateObject();
  cJSON_AddItemToObject(capabilities, "tools", tools_cap);
  cJSON_AddItemToObject(result, "capabilities", capabilities);

  // Server info
  cJSON* server_info = cJSON_CreateObject();
  cJSON_AddStringToObject(server_info, "name", "ESP32 MCP Server");
  cJSON_AddStringToObject(server_info, "version", "1.0.0");
  cJSON_AddItemToObject(result, "serverInfo", server_info);

  // Protocol version
  cJSON_AddStringToObject(result, "protocolVersion",
                          server->protocol_version ? server->protocol_version : "2024-11-05");

  // Send response
  char* response = mcp_protocol_create_response(request_id, result);
  if (response) {
    server->transport->vtable->send_response(server->transport, response);
    free(response);
  }

  ESP_LOGI(TAG, "Initialize response sent");
}

static void handle_initialized(mcp_server_t* server) {
  ESP_LOGI(TAG, "Client sent initialized notification");
  server->is_initialized = true;
}

static tool_entry_t* find_tool(mcp_server_t* server, const char* name) {
  if (!server || !name) {
    return NULL;
  }

  for (size_t i = 0; i < server->tool_count; i++) {
    if (strcmp(server->tools[i].name, name) == 0) {
      return &server->tools[i];
    }
  }

  return NULL;
}

static void handle_request(const char* request, void* context) {
  mcp_server_t* server = (mcp_server_t*)context;
  if (!server || !request) {
    return;
  }

  ESP_LOGD(TAG, "Handling request: %s", request);

  // Parse request
  mcp_request_t req;
  esp_err_t ret = mcp_protocol_parse_request(request, &req);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to parse request");
    char* error_resp = mcp_protocol_create_error(0, -32700, "Parse error");
    if (error_resp) {
      server->transport->vtable->send_response(server->transport, error_resp);
      free(error_resp);
    }
    return;
  }

  // Route based on method
  if (strcmp(req.method, "initialize") == 0) {
    handle_initialize(server, req.params, req.id);
  }
  else if (strcmp(req.method, "notifications/initialized") == 0) {
    handle_initialized(server);
    // Notifications don't get JSON-RPC responses, but HTTP needs 200 OK
    // Send empty response to satisfy HTTP
    server->transport->vtable->send_response(server->transport, "{}");
    mcp_protocol_free_request(&req);
    return;
  }
  else if (strcmp(req.method, "tools/list") == 0) {
    handle_list_tools(server, req.id);
  }
  else if (strcmp(req.method, "tools/call") == 0) {
    // Extract tool name and arguments
    const char* tool_name = NULL;
    cJSON* arguments = NULL;

    if (req.params) {
      cJSON* name_item = cJSON_GetObjectItem(req.params, "name");
      if (name_item && cJSON_IsString(name_item)) {
        tool_name = name_item->valuestring;
      }

      arguments = cJSON_GetObjectItem(req.params, "arguments");
    }

    if (!tool_name) {
      char* error_resp = mcp_protocol_create_error(req.id, -32602, "Missing 'name' parameter");
      if (error_resp) {
        server->transport->vtable->send_response(server->transport, error_resp);
        free(error_resp);
      }
    }
    else {
      handle_call_tool(server, tool_name, arguments, req.id);
    }
  }
  else {
    ESP_LOGW(TAG, "Unknown method: %s", req.method);
    char* error_resp = mcp_protocol_create_error(req.id, -32601, "Method not found");
    if (error_resp) {
      server->transport->vtable->send_response(server->transport, error_resp);
      free(error_resp);
    }
  }

  mcp_protocol_free_request(&req);
}

static void handle_list_tools(mcp_server_t* server, int request_id) {
  // Create tools array
  cJSON* tools_array = cJSON_CreateArray();
  if (!tools_array) {
    char* error_resp = mcp_protocol_create_error(request_id, -32603, "Internal error");
    if (error_resp) {
      server->transport->vtable->send_response(server->transport, error_resp);
      free(error_resp);
    }
    return;
  }

  for (size_t i = 0; i < server->tool_count; i++) {
    cJSON* tool_obj = cJSON_CreateObject();
    if (tool_obj) {
      cJSON_AddStringToObject(tool_obj, "name", server->tools[i].name);
      if (server->tools[i].description) {
        cJSON_AddStringToObject(tool_obj, "description", server->tools[i].description);
      }

      // Generate input schema from parameters
      cJSON* schema;
      if (server->tools[i].parameters && server->tools[i].parameter_count > 0) {
        // Use schema generation for tools with parameters
        schema = mcp_schema_to_json(server->tools[i].parameters, server->tools[i].parameter_count);
      }
      else {
        // Fallback to empty schema for tools without parameters
        schema = cJSON_CreateObject();
        cJSON_AddStringToObject(schema, "type", "object");
        cJSON_AddObjectToObject(schema, "properties");
      }

      if (schema) {
        cJSON_AddItemToObject(tool_obj, "inputSchema", schema);
      }

      cJSON_AddItemToArray(tools_array, tool_obj);
    }
  }

  // Create result object
  cJSON* result = cJSON_CreateObject();
  cJSON_AddItemToObject(result, "tools", tools_array);

  // Create response
  char* response = mcp_protocol_create_response(request_id, result);
  if (response) {
    server->transport->vtable->send_response(server->transport, response);
    free(response);
  }
}

static void handle_call_tool(mcp_server_t* server, const char* tool_name, cJSON* args, int request_id) {
  // Allow tool calls even if initialized notification wasn't sent
  // Some clients (like our test script) skip the full handshake

  // Find tool
  tool_entry_t* tool = find_tool(server, tool_name);
  if (!tool) {
    ESP_LOGW(TAG, "Tool not found: %s", tool_name);
    char* error_resp = mcp_protocol_create_error(request_id, -32602, "Tool not found");
    if (error_resp) {
      server->transport->vtable->send_response(server->transport, error_resp);
      free(error_resp);
    }
    return;
  }

  // Call tool handler
  ESP_LOGD(TAG, "Calling tool '%s'", tool_name);
  if (args) {
    char* args_str = cJSON_Print(args);
    ESP_LOGD(TAG, "Tool arguments: %s", args_str ? args_str : "NULL");
    if (args_str) free(args_str);
  }
  else {
    ESP_LOGD(TAG, "Tool arguments: NULL");
  }

  mcp_tool_args_t tool_args = {.json = args};
  mcp_tool_result_t result = tool->handler(&tool_args);

  ESP_LOGD(TAG, "Tool '%s' completed: %s", tool_name, result.success ? "SUCCESS" : "ERROR");
  if (!result.success && result.error_message) {
    ESP_LOGD(TAG, "Error message: %s", result.error_message);
  }

  // Convert result to JSON and send response
  cJSON* result_json = mcp_tool_result_to_json(&result);
  char* response = mcp_protocol_create_response(request_id, result_json);

  if (response) {
    server->transport->vtable->send_response(server->transport, response);
    free(response);
  }

  mcp_tool_result_free(&result);
}
