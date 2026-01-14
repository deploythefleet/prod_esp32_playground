#ifndef MCP_TYPES_H
#define MCP_TYPES_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCP transport types supported by the server
 */
typedef enum {
  MCP_TRANSPORT_HTTP = 0,  ///< HTTP/HTTPS transport
  MCP_TRANSPORT_UART,      ///< UART/Serial transport (future)
  MCP_TRANSPORT_WEBSOCKET  ///< WebSocket transport (future)
} mcp_transport_type_t;

/**
 * @brief MCP error codes
 */
#define MCP_OK ESP_OK
#define MCP_ERR_INVALID_ARG ESP_ERR_INVALID_ARG
#define MCP_ERR_NO_MEM ESP_ERR_NO_MEM
#define MCP_ERR_NOT_FOUND ESP_ERR_NOT_FOUND
#define MCP_ERR_INVALID_STATE ESP_ERR_INVALID_STATE
#define MCP_ERR_TIMEOUT ESP_ERR_TIMEOUT
#define MCP_ERR_TRANSPORT (ESP_ERR_INVALID_STATE + 1)
#define MCP_ERR_PROTOCOL (ESP_ERR_INVALID_STATE + 2)
#define MCP_ERR_TOOL_EXECUTION (ESP_ERR_INVALID_STATE + 3)

#ifdef __cplusplus
}
#endif

#endif  // MCP_TYPES_H
