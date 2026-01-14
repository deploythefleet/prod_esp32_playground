#include "mcp_schema.h"

#include <string.h>

#include "esp_log.h"

static const char* TAG = "mcp_schema";

/**
 * @brief Get string representation of parameter type
 */
static const char* param_type_to_string(mcp_param_type_t type) {
  switch (type) {
    case MCP_TYPE_STRING:
      return "string";
    case MCP_TYPE_NUMBER:
      return "number";
    case MCP_TYPE_INTEGER:
      return "integer";
    case MCP_TYPE_BOOLEAN:
      return "boolean";
    case MCP_TYPE_OBJECT:
      return "object";
    case MCP_TYPE_ARRAY:
      return "array";
    case MCP_TYPE_NULL:
      return "null";
    default:
      return "string";
  }
}

cJSON* mcp_schema_param_to_json(const mcp_param_schema_t* schema) {
  if (!schema || !schema->name) {
    ESP_LOGE(TAG, "Invalid schema parameter");
    return NULL;
  }

  cJSON* param = cJSON_CreateObject();
  if (!param) {
    ESP_LOGE(TAG, "Failed to create parameter JSON object");
    return NULL;
  }

  // Add type
  cJSON_AddStringToObject(param, "type", param_type_to_string(schema->type));

  // Add description if present
  if (schema->description) {
    cJSON_AddStringToObject(param, "description", schema->description);
  }

  // Add numeric constraints
  if (schema->type == MCP_TYPE_NUMBER || schema->type == MCP_TYPE_INTEGER) {
    if (schema->has_minimum) {
      if (schema->type == MCP_TYPE_INTEGER) {
        cJSON_AddNumberToObject(param, "minimum", (int)schema->minimum);
      }
      else {
        cJSON_AddNumberToObject(param, "minimum", schema->minimum);
      }
    }
    if (schema->has_maximum) {
      if (schema->type == MCP_TYPE_INTEGER) {
        cJSON_AddNumberToObject(param, "maximum", (int)schema->maximum);
      }
      else {
        cJSON_AddNumberToObject(param, "maximum", schema->maximum);
      }
    }
  }

  // Add string constraints
  if (schema->type == MCP_TYPE_STRING) {
    if (schema->has_min_length) {
      cJSON_AddNumberToObject(param, "minLength", schema->min_length);
    }
    if (schema->has_max_length) {
      cJSON_AddNumberToObject(param, "maxLength", schema->max_length);
    }
    if (schema->pattern) {
      cJSON_AddStringToObject(param, "pattern", schema->pattern);
    }
  }

  // Add enum values if present
  if (schema->enum_values && schema->enum_count > 0) {
    cJSON* enum_array = cJSON_CreateArray();
    if (enum_array) {
      for (size_t i = 0; i < schema->enum_count; i++) {
        cJSON_AddItemToArray(enum_array, cJSON_CreateString(schema->enum_values[i]));
      }
      cJSON_AddItemToObject(param, "enum", enum_array);
    }
  }

  return param;
}

cJSON* mcp_schema_to_json(const mcp_param_schema_t* parameters, size_t count) {
  // Create root schema object
  cJSON* schema = cJSON_CreateObject();
  if (!schema) {
    ESP_LOGE(TAG, "Failed to create schema JSON object");
    return NULL;
  }

  // Add type: "object"
  cJSON_AddStringToObject(schema, "type", "object");

  // Create properties object
  cJSON* properties = cJSON_CreateObject();
  if (!properties) {
    ESP_LOGE(TAG, "Failed to create properties JSON object");
    cJSON_Delete(schema);
    return NULL;
  }

  // Create required array
  cJSON* required = cJSON_CreateArray();
  if (!required) {
    ESP_LOGE(TAG, "Failed to create required JSON array");
    cJSON_Delete(properties);
    cJSON_Delete(schema);
    return NULL;
  }

  // Process each parameter
  for (size_t i = 0; i < count; i++) {
    const mcp_param_schema_t* param = &parameters[i];

    // Convert parameter to JSON and add to properties
    cJSON* param_json = mcp_schema_param_to_json(param);
    if (param_json) {
      cJSON_AddItemToObject(properties, param->name, param_json);

      // Add to required array if needed
      if (param->required) {
        cJSON_AddItemToArray(required, cJSON_CreateString(param->name));
      }
    }
    else {
      ESP_LOGW(TAG, "Failed to convert parameter '%s' to JSON", param->name ? param->name : "unknown");
    }
  }

  // Add properties and required to schema
  cJSON_AddItemToObject(schema, "properties", properties);
  cJSON_AddItemToObject(schema, "required", required);

  return schema;
}
