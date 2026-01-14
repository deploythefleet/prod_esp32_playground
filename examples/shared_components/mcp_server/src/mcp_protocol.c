#include "mcp_protocol.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

static const char* TAG = "mcp_protocol";

esp_err_t mcp_protocol_parse_request(const char* json_str, mcp_request_t* request) {
  if (!json_str || !request) {
    return ESP_ERR_INVALID_ARG;
  }

  memset(request, 0, sizeof(mcp_request_t));

  cJSON* root = cJSON_Parse(json_str);
  if (!root) {
    ESP_LOGE(TAG, "Failed to parse JSON request");
    return ESP_FAIL;
  }

  // Parse jsonrpc version
  cJSON* jsonrpc = cJSON_GetObjectItem(root, "jsonrpc");
  if (jsonrpc && cJSON_IsString(jsonrpc)) {
    request->jsonrpc = strdup(jsonrpc->valuestring);
  }

  // Parse id (optional)
  cJSON* id = cJSON_GetObjectItem(root, "id");
  if (id && cJSON_IsNumber(id)) {
    request->id = id->valueint;
    request->id_is_valid = true;
    request->is_notification = false;
  }
  else {
    // No id means this is a notification
    request->is_notification = true;
  }

  // Parse method
  cJSON* method = cJSON_GetObjectItem(root, "method");
  if (method && cJSON_IsString(method)) {
    request->method = strdup(method->valuestring);
  }
  else {
    ESP_LOGE(TAG, "Missing or invalid 'method' in request");
    cJSON_Delete(root);
    mcp_protocol_free_request(request);
    return ESP_FAIL;
  }

  // Parse params (optional)
  cJSON* params = cJSON_GetObjectItem(root, "params");
  if (params) {
    // Duplicate the params object so we can free the root
    request->params = cJSON_Duplicate(params, true);
  }

  cJSON_Delete(root);
  return ESP_OK;
}

void mcp_protocol_free_request(mcp_request_t* request) {
  if (!request) {
    return;
  }

  if (request->jsonrpc) {
    free(request->jsonrpc);
    request->jsonrpc = NULL;
  }

  if (request->method) {
    free(request->method);
    request->method = NULL;
  }

  if (request->params) {
    cJSON_Delete(request->params);
    request->params = NULL;
  }

  request->id_is_valid = false;
}

char* mcp_protocol_create_response(int id, cJSON* result) {
  cJSON* root = cJSON_CreateObject();
  if (!root) {
    ESP_LOGE(TAG, "Failed to create response object");
    return NULL;
  }

  cJSON_AddStringToObject(root, "jsonrpc", "2.0");
  cJSON_AddNumberToObject(root, "id", id);

  if (result) {
    cJSON_AddItemToObject(root, "result", result);
  }
  else {
    cJSON_AddNullToObject(root, "result");
  }

  char* response_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  return response_str;
}

char* mcp_protocol_create_error(int id, int code, const char* message) {
  cJSON* root = cJSON_CreateObject();
  if (!root) {
    ESP_LOGE(TAG, "Failed to create error object");
    return NULL;
  }

  cJSON_AddStringToObject(root, "jsonrpc", "2.0");
  cJSON_AddNumberToObject(root, "id", id);

  cJSON* error = cJSON_CreateObject();
  if (error) {
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message ? message : "Unknown error");
    cJSON_AddItemToObject(root, "error", error);
  }

  char* error_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  return error_str;
}
