#include "mcp_tool.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

static const char* TAG = "mcp_tool";

const char* mcp_tool_args_get_string(const mcp_tool_args_t* args, const char* key, const char* default_val) {
  if (!args || !args->json || !key) {
    return default_val;
  }

  cJSON* item = cJSON_GetObjectItem(args->json, key);
  if (!item || !cJSON_IsString(item)) {
    return default_val;
  }

  return item->valuestring;
}

int mcp_tool_args_get_int(const mcp_tool_args_t* args, const char* key, int default_val) {
  if (!args || !args->json || !key) {
    return default_val;
  }

  cJSON* item = cJSON_GetObjectItem(args->json, key);
  if (!item || !cJSON_IsNumber(item)) {
    return default_val;
  }

  return item->valueint;
}

bool mcp_tool_args_get_bool(const mcp_tool_args_t* args, const char* key, bool default_val) {
  if (!args || !args->json || !key) {
    return default_val;
  }

  cJSON* item = cJSON_GetObjectItem(args->json, key);
  if (!item || !cJSON_IsBool(item)) {
    return default_val;
  }

  return cJSON_IsTrue(item);
}

double mcp_tool_args_get_double(const mcp_tool_args_t* args, const char* key, double default_val) {
  if (!args || !args->json || !key) {
    return default_val;
  }

  cJSON* item = cJSON_GetObjectItem(args->json, key);
  if (!item || !cJSON_IsNumber(item)) {
    return default_val;
  }

  return item->valuedouble;
}

mcp_tool_result_t mcp_tool_result_success(const char* content) {
  mcp_tool_result_t result = {0};
  result.success = true;

  if (content) {
    result.content = strdup(content);
    result._content_allocated = true;
    if (!result.content) {
      ESP_LOGE(TAG, "Failed to allocate memory for success content");
      result.success = false;
      result.error_message = strdup("Memory allocation failed");
      result._error_allocated = true;
    }
  }

  return result;
}

mcp_tool_result_t mcp_tool_result_error(const char* error_message) {
  mcp_tool_result_t result = {0};
  result.success = false;

  if (error_message) {
    result.error_message = strdup(error_message);
    result._error_allocated = true;
    if (!result.error_message) {
      ESP_LOGE(TAG, "Failed to allocate memory for error message");
    }
  }

  return result;
}

void mcp_tool_result_free(mcp_tool_result_t* result) {
  if (!result) {
    return;
  }

  if (result->_content_allocated && result->content) {
    free(result->content);
    result->content = NULL;
  }

  if (result->_error_allocated && result->error_message) {
    free(result->error_message);
    result->error_message = NULL;
  }

  result->_content_allocated = false;
  result->_error_allocated = false;
}

cJSON* mcp_tool_result_to_json(const mcp_tool_result_t* result) {
  if (!result) {
    return NULL;
  }

  cJSON* root = cJSON_CreateObject();
  if (!root) {
    return NULL;
  }

  if (result->success) {
    // Success response format: { "content": [{ "type": "text", "text": "..." }] }
    cJSON* content_array = cJSON_CreateArray();
    if (content_array) {
      cJSON* text_obj = cJSON_CreateObject();
      if (text_obj) {
        cJSON_AddStringToObject(text_obj, "type", "text");
        cJSON_AddStringToObject(text_obj, "text", result->content ? result->content : "");
        cJSON_AddItemToArray(content_array, text_obj);
      }
      cJSON_AddItemToObject(root, "content", content_array);
    }
  }
  else {
    // Error response format: { "error": "..." }
    cJSON_AddStringToObject(root, "error", result->error_message ? result->error_message : "Unknown error");
  }

  return root;
}
