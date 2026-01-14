#ifndef MCP_SCHEMA_H
#define MCP_SCHEMA_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameter type enumeration
 */
typedef enum {
  MCP_TYPE_STRING,
  MCP_TYPE_NUMBER,
  MCP_TYPE_INTEGER,
  MCP_TYPE_BOOLEAN,
  MCP_TYPE_OBJECT,
  MCP_TYPE_ARRAY,
  MCP_TYPE_NULL
} mcp_param_type_t;

/**
 * @brief Parameter schema definition
 */
typedef struct {
  const char* name;         ///< Parameter name
  mcp_param_type_t type;    ///< Parameter type
  const char* description;  ///< Human-readable description
  bool required;            ///< Whether parameter is required

  // Numeric constraints (for NUMBER and INTEGER types)
  double minimum;    ///< Minimum value (inclusive)
  double maximum;    ///< Maximum value (inclusive)
  bool has_minimum;  ///< Whether minimum constraint is set
  bool has_maximum;  ///< Whether maximum constraint is set

  // String constraints (for STRING type)
  int min_length;       ///< Minimum string length
  int max_length;       ///< Maximum string length
  const char* pattern;  ///< Regex pattern (optional)
  bool has_min_length;  ///< Whether min_length constraint is set
  bool has_max_length;  ///< Whether max_length constraint is set

  // Enum constraints (for any type)
  const char** enum_values;  ///< Array of allowed values as strings
  size_t enum_count;         ///< Number of enum values
} mcp_param_schema_t;

/**
 * @brief Convert parameter schema to JSON object
 *
 * @param schema Parameter schema
 * @return cJSON object representing the parameter schema (must be freed by caller)
 */
cJSON* mcp_schema_param_to_json(const mcp_param_schema_t* schema);

/**
 * @brief Convert array of parameter schemas to JSON Schema inputSchema object
 *
 * Creates a JSON Schema object with type: "object", properties, and required array.
 *
 * @param parameters Array of parameter schemas
 * @param count Number of parameters
 * @return cJSON object representing the full input schema (must be freed by caller)
 */
cJSON* mcp_schema_to_json(const mcp_param_schema_t* parameters, size_t count);

// ====================================================================================
// Helper Macros for Schema Definition
// ====================================================================================

/**
 * @brief Define a required string parameter
 */
#define MCP_PARAM_STRING_REQUIRED(param_name, desc) \
  {.name = param_name,                              \
   .type = MCP_TYPE_STRING,                         \
   .description = desc,                             \
   .required = true,                                \
   .has_minimum = false,                            \
   .has_maximum = false,                            \
   .has_min_length = false,                         \
   .has_max_length = false,                         \
   .enum_values = NULL,                             \
   .enum_count = 0}

/**
 * @brief Define an optional string parameter
 */
#define MCP_PARAM_STRING(param_name, desc) \
  {.name = param_name,                     \
   .type = MCP_TYPE_STRING,                \
   .description = desc,                    \
   .required = false,                      \
   .has_minimum = false,                   \
   .has_maximum = false,                   \
   .has_min_length = false,                \
   .has_max_length = false,                \
   .enum_values = NULL,                    \
   .enum_count = 0}

/**
 * @brief Define a required number parameter with min/max constraints
 */
#define MCP_PARAM_NUMBER_REQUIRED(param_name, desc, min_val, max_val) \
  {.name = param_name,                                                \
   .type = MCP_TYPE_NUMBER,                                           \
   .description = desc,                                               \
   .required = true,                                                  \
   .minimum = min_val,                                                \
   .maximum = max_val,                                                \
   .has_minimum = true,                                               \
   .has_maximum = true,                                               \
   .has_min_length = false,                                           \
   .has_max_length = false,                                           \
   .enum_values = NULL,                                               \
   .enum_count = 0}

/**
 * @brief Define an optional number parameter with min/max constraints
 */
#define MCP_PARAM_NUMBER(param_name, desc, min_val, max_val) \
  {.name = param_name,                                       \
   .type = MCP_TYPE_NUMBER,                                  \
   .description = desc,                                      \
   .required = false,                                        \
   .minimum = min_val,                                       \
   .maximum = max_val,                                       \
   .has_minimum = true,                                      \
   .has_maximum = true,                                      \
   .has_min_length = false,                                  \
   .has_max_length = false,                                  \
   .enum_values = NULL,                                      \
   .enum_count = 0}

/**
 * @brief Define a required integer parameter with min/max constraints
 */
#define MCP_PARAM_INTEGER_REQUIRED(param_name, desc, min_val, max_val) \
  {.name = param_name,                                                 \
   .type = MCP_TYPE_INTEGER,                                           \
   .description = desc,                                                \
   .required = true,                                                   \
   .minimum = min_val,                                                 \
   .maximum = max_val,                                                 \
   .has_minimum = true,                                                \
   .has_maximum = true,                                                \
   .has_min_length = false,                                            \
   .has_max_length = false,                                            \
   .enum_values = NULL,                                                \
   .enum_count = 0}

/**
 * @brief Define an optional integer parameter with min/max constraints
 */
#define MCP_PARAM_INTEGER(param_name, desc, min_val, max_val) \
  {.name = param_name,                                        \
   .type = MCP_TYPE_INTEGER,                                  \
   .description = desc,                                       \
   .required = false,                                         \
   .minimum = min_val,                                        \
   .maximum = max_val,                                        \
   .has_minimum = true,                                       \
   .has_maximum = true,                                       \
   .has_min_length = false,                                   \
   .has_max_length = false,                                   \
   .enum_values = NULL,                                       \
   .enum_count = 0}

/**
 * @brief Define a required boolean parameter
 */
#define MCP_PARAM_BOOLEAN_REQUIRED(param_name, desc) \
  {.name = param_name,                               \
   .type = MCP_TYPE_BOOLEAN,                         \
   .description = desc,                              \
   .required = true,                                 \
   .has_minimum = false,                             \
   .has_maximum = false,                             \
   .has_min_length = false,                          \
   .has_max_length = false,                          \
   .enum_values = NULL,                              \
   .enum_count = 0}

/**
 * @brief Define an optional boolean parameter
 */
#define MCP_PARAM_BOOLEAN(param_name, desc) \
  {.name = param_name,                      \
   .type = MCP_TYPE_BOOLEAN,                \
   .description = desc,                     \
   .required = false,                       \
   .has_minimum = false,                    \
   .has_maximum = false,                    \
   .has_min_length = false,                 \
   .has_max_length = false,                 \
   .enum_values = NULL,                     \
   .enum_count = 0}

#ifdef __cplusplus
}
#endif

#endif  // MCP_SCHEMA_H
