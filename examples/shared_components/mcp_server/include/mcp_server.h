#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <stdbool.h>
#include <stddef.h>

#include "mcp_tool.h"
#include "mcp_transport.h"
#include "mcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct mcp_server mcp_server_t;

/**
 * @brief Create an MCP server instance
 *
 * @param transport_type Type of transport to use
 * @return Server instance or NULL on failure (must be destroyed with mcp_server_destroy)
 */
mcp_server_t* mcp_server_create(mcp_transport_type_t transport_type);

/**
 * @brief Destroy the MCP server and free resources
 *
 * @param server Server instance
 */
void mcp_server_destroy(mcp_server_t* server);

/**
 * @brief Register a tool (imperative API)
 *
 * @param server Server instance
 * @param name Tool name (must be unique)
 * @param description Tool description
 * @param handler Tool handler function
 * @return ESP_OK on success
 */
esp_err_t mcp_server_add_tool(mcp_server_t* server, const char* name, const char* description,
                              mcp_tool_handler_t handler);

/**
 * @brief Register a tool (declarative API)
 *
 * @param server Server instance
 * @param tool Tool definition
 * @return ESP_OK on success
 */
esp_err_t mcp_server_register_tool(mcp_server_t* server, const mcp_tool_definition_t* tool);

/**
 * @brief Register multiple tools at once (declarative API)
 *
 * @param server Server instance
 * @param tools Array of tool definition pointers
 * @param count Number of tools
 * @return ESP_OK on success
 */
esp_err_t mcp_server_register_tools(mcp_server_t* server, const mcp_tool_definition_t* tools[], size_t count);

/**
 * @brief Start the MCP server
 *
 * @param server Server instance
 * @param port Port number for transport (e.g., HTTP port)
 * @return ESP_OK on success
 */
esp_err_t mcp_server_start(mcp_server_t* server, uint16_t port);

/**
 * @brief Stop the MCP server
 *
 * @param server Server instance
 * @return ESP_OK on success
 */
esp_err_t mcp_server_stop(mcp_server_t* server);

/**
 * @brief Get the number of registered tools
 *
 * @param server Server instance
 * @return Number of tools
 */
size_t mcp_server_get_tool_count(const mcp_server_t* server);

/**
 * @brief Check if a tool exists
 *
 * @param server Server instance
 * @param name Tool name
 * @return true if tool exists
 */
bool mcp_server_has_tool(const mcp_server_t* server, const char* name);

#ifdef __cplusplus
}
#endif

#endif  // MCP_SERVER_H
