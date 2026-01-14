#ifndef MCP_PROTOCOL_H
#define MCP_PROTOCOL_H

#include <stdbool.h>

#include "cJSON.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCP JSON-RPC request structure
 */
typedef struct {
  char* jsonrpc;         ///< JSON-RPC version (should be "2.0")
  int id;                ///< Request ID
  char* method;          ///< Method name (e.g., "tools/list", "tools/call")
  cJSON* params;         ///< Parameters object
  bool id_is_valid;      ///< Whether ID was present in request
  bool is_notification;  ///< True if this is a notification (no id, no response expected)
} mcp_request_t;

/**
 * @brief Parse JSON-RPC request
 *
 * @param json_str JSON string
 * @param request Output request structure
 * @return ESP_OK on success
 */
esp_err_t mcp_protocol_parse_request(const char* json_str, mcp_request_t* request);

/**
 * @brief Free resources allocated by parse_request
 *
 * @param request Request structure
 */
void mcp_protocol_free_request(mcp_request_t* request);

/**
 * @brief Create a JSON-RPC success response
 *
 * @param id Request ID
 * @param result Result JSON object (will be added to response, do not free)
 * @return JSON string (must be freed by caller)
 */
char* mcp_protocol_create_response(int id, cJSON* result);

/**
 * @brief Create a JSON-RPC error response
 *
 * @param id Request ID
 * @param code Error code
 * @param message Error message
 * @return JSON string (must be freed by caller)
 */
char* mcp_protocol_create_error(int id, int code, const char* message);

#ifdef __cplusplus
}
#endif

#endif  // MCP_PROTOCOL_H
