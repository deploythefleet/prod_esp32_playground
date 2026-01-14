#ifndef MCP_TRANSPORT_H
#define MCP_TRANSPORT_H

#include <stdint.h>

#include "esp_err.h"
#include "mcp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct mcp_transport mcp_transport_t;

/**
 * @brief Callback for handling incoming requests
 *
 * @param request JSON-RPC request string
 * @param context User context passed during registration
 */
typedef void (*mcp_request_handler_t)(const char* request, void* context);

/**
 * @brief Transport virtual function table
 */
typedef struct {
  /**
   * @brief Initialize the transport
   *
   * @param transport Transport instance
   * @param port Port number (interpretation depends on transport)
   * @return ESP_OK on success
   */
  esp_err_t (*init)(mcp_transport_t* transport, uint16_t port);

  /**
   * @brief Start the transport (begin listening for requests)
   *
   * @param transport Transport instance
   * @return ESP_OK on success
   */
  esp_err_t (*start)(mcp_transport_t* transport);

  /**
   * @brief Stop the transport
   *
   * @param transport Transport instance
   * @return ESP_OK on success
   */
  esp_err_t (*stop)(mcp_transport_t* transport);

  /**
   * @brief Send response back to client
   *
   * @param transport Transport instance
   * @param response JSON-RPC response string
   * @return ESP_OK on success
   */
  esp_err_t (*send_response)(mcp_transport_t* transport, const char* response);

  /**
   * @brief Destroy the transport and free resources
   *
   * @param transport Transport instance
   */
  void (*destroy)(mcp_transport_t* transport);
} mcp_transport_vtable_t;

/**
 * @brief Base transport structure
 */
struct mcp_transport {
  const mcp_transport_vtable_t* vtable;   ///< Virtual function table
  mcp_request_handler_t request_handler;  ///< Request handler callback
  void* handler_context;                  ///< Context for request handler
  void* impl_data;                        ///< Transport-specific implementation data
};

/**
 * @brief Set the request handler callback
 *
 * @param transport Transport instance
 * @param handler Request handler function
 * @param context User context to pass to handler
 */
void mcp_transport_set_request_handler(mcp_transport_t* transport, mcp_request_handler_t handler, void* context);

/**
 * @brief Invoke the request handler (called by transport implementations)
 *
 * @param transport Transport instance
 * @param request JSON-RPC request string
 */
void mcp_transport_invoke_handler(mcp_transport_t* transport, const char* request);

#ifdef __cplusplus
}
#endif

#endif  // MCP_TRANSPORT_H
