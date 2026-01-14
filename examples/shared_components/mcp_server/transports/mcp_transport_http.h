#ifndef MCP_TRANSPORT_HTTP_H
#define MCP_TRANSPORT_HTTP_H

#include "mcp_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an HTTP transport instance
 *
 * @return Transport instance or NULL on failure (must be destroyed with vtable->destroy)
 */
mcp_transport_t* mcp_transport_http_create(void);

#ifdef __cplusplus
}
#endif

#endif  // MCP_TRANSPORT_HTTP_H
