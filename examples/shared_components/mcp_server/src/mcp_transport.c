#include "mcp_transport.h"

#include <string.h>

void mcp_transport_set_request_handler(mcp_transport_t* transport, mcp_request_handler_t handler, void* context) {
  if (!transport) {
    return;
  }

  transport->request_handler = handler;
  transport->handler_context = context;
}

void mcp_transport_invoke_handler(mcp_transport_t* transport, const char* request) {
  if (!transport || !transport->request_handler || !request) {
    return;
  }

  transport->request_handler(request, transport->handler_context);
}
