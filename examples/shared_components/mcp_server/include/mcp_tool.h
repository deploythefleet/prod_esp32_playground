#ifndef MCP_TOOL_H
#define MCP_TOOL_H

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"
#include "mcp_schema.h"
#include "mcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Arguments passed to a tool handler
 */
typedef struct {
  cJSON* json;  ///< Raw JSON arguments object
} mcp_tool_args_t;

/**
 * @brief Result returned from a tool handler
 */
typedef struct {
  bool success;             ///< Whether the tool execution succeeded
  char* content;            ///< Content/result text (owned by struct)
  char* error_message;      ///< Error message if failed (owned by struct)
  bool _content_allocated;  ///< Internal: whether content was allocated
  bool _error_allocated;    ///< Internal: whether error was allocated
} mcp_tool_result_t;

/**
 * @brief Tool handler function signature
 *
 * @param args Tool arguments
 * @return Tool result (caller must free with mcp_tool_result_free)
 */
typedef mcp_tool_result_t (*mcp_tool_handler_t)(const mcp_tool_args_t* args);

/**
 * @brief Tool definition structure
 */
typedef struct {
  const char* name;                      ///< Tool name (must be unique)
  const char* description;               ///< Human-readable description
  mcp_tool_handler_t handler;            ///< Handler function
  const mcp_param_schema_t* parameters;  ///< Array of parameter schemas (optional)
  size_t parameter_count;                ///< Number of parameters
} mcp_tool_definition_t;

/**
 * @brief Helper: Get string value from tool arguments
 *
 * @param args Tool arguments
 * @param key JSON key to retrieve
 * @param default_val Default value if key not found
 * @return String value or default
 */
const char* mcp_tool_args_get_string(const mcp_tool_args_t* args, const char* key, const char* default_val);

/**
 * @brief Helper: Get integer value from tool arguments
 *
 * @param args Tool arguments
 * @param key JSON key to retrieve
 * @param default_val Default value if key not found
 * @return Integer value or default
 */
int mcp_tool_args_get_int(const mcp_tool_args_t* args, const char* key, int default_val);

/**
 * @brief Helper: Get boolean value from tool arguments
 *
 * @param args Tool arguments
 * @param key JSON key to retrieve
 * @param default_val Default value if key not found
 * @return Boolean value or default
 */
bool mcp_tool_args_get_bool(const mcp_tool_args_t* args, const char* key, bool default_val);

/**
 * @brief Helper: Get double value from tool arguments
 *
 * @param args Tool arguments
 * @param key JSON key to retrieve
 * @param default_val Default value if key not found
 * @return Double value or default
 */
double mcp_tool_args_get_double(const mcp_tool_args_t* args, const char* key, double default_val);

/**
 * @brief Create a success result
 *
 * @param content Success message/content (will be copied)
 * @return Tool result (must be freed with mcp_tool_result_free)
 */
mcp_tool_result_t mcp_tool_result_success(const char* content);

/**
 * @brief Create an error result
 *
 * @param error_message Error message (will be copied)
 * @return Tool result (must be freed with mcp_tool_result_free)
 */
mcp_tool_result_t mcp_tool_result_error(const char* error_message);

/**
 * @brief Free resources allocated by a tool result
 *
 * @param result Tool result to free
 */
void mcp_tool_result_free(mcp_tool_result_t* result);

/**
 * @brief Convert tool result to JSON format
 *
 * @param result Tool result
 * @return cJSON object (caller must free with cJSON_Delete)
 */
cJSON* mcp_tool_result_to_json(const mcp_tool_result_t* result);

#ifdef __cplusplus
}
#endif

#endif  // MCP_TOOL_H
