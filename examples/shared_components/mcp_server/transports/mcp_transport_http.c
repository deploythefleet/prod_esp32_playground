#include "mcp_transport_http.h"

#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

static const char* TAG = "mcp_http";

#define MAX_REQUEST_SIZE 4096

/**
 * @brief HTTP transport implementation data
 */
typedef struct {
  httpd_handle_t server;
  uint16_t port;
  char* pending_response;  // Response to send back
} mcp_http_impl_t;

// Forward declarations
static esp_err_t http_init(mcp_transport_t* transport, uint16_t port);
static esp_err_t http_start(mcp_transport_t* transport);
static esp_err_t http_stop(mcp_transport_t* transport);
static esp_err_t http_send_response(mcp_transport_t* transport, const char* response);
static void http_destroy(mcp_transport_t* transport);
static esp_err_t mcp_http_options_handler(httpd_req_t* req);

// HTTP handler for POST requests
static esp_err_t mcp_http_post_handler(httpd_req_t* req) {
  mcp_transport_t* transport = (mcp_transport_t*)req->user_ctx;
  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;

  // Allocate buffer for request body
  char* request_buf = malloc(MAX_REQUEST_SIZE);
  if (!request_buf) {
    ESP_LOGE(TAG, "Failed to allocate request buffer");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  // Read request body
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;

  if (total_len >= MAX_REQUEST_SIZE) {
    ESP_LOGW(TAG, "Request too large: %d bytes", total_len);
    free(request_buf);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Request too large");
    return ESP_FAIL;
  }

  while (cur_len < total_len) {
    received = httpd_req_recv(req, request_buf + cur_len, total_len - cur_len);
    if (received <= 0) {
      ESP_LOGE(TAG, "Failed to receive request body");
      free(request_buf);
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    cur_len += received;
  }
  request_buf[total_len] = '\0';

  ESP_LOGD(TAG, "=== INCOMING HTTP REQUEST ===");
  ESP_LOGD(TAG, "Content-Length: %d", total_len);
  ESP_LOGD(TAG, "Request body: %s", request_buf);
  ESP_LOGD(TAG, "==============================");

  // Clear previous response
  if (impl->pending_response) {
    free(impl->pending_response);
    impl->pending_response = NULL;
  }

  // Invoke the request handler (which will call send_response)
  mcp_transport_invoke_handler(transport, request_buf);

  // Send the response
  if (impl->pending_response) {
    ESP_LOGD(TAG, "=== OUTGOING HTTP RESPONSE ===");
    ESP_LOGD(TAG, "Response body: %s", impl->pending_response);
    ESP_LOGD(TAG, "================================");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, impl->pending_response);
    free(impl->pending_response);
    impl->pending_response = NULL;
  }
  else {
    // No response set, send error
    ESP_LOGW(TAG, "No response generated");
    httpd_resp_send_500(req);
  }

  free(request_buf);
  return ESP_OK;
}

// OPTIONS handler for CORS preflight
static esp_err_t mcp_http_options_handler(httpd_req_t* req) {
  // Set CORS headers
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
  httpd_resp_set_hdr(req, "Access-Control-Max-Age", "86400");

  // Send 204 No Content
  httpd_resp_set_status(req, "204 No Content");
  httpd_resp_send(req, NULL, 0);

  return ESP_OK;
}

static esp_err_t http_init(mcp_transport_t* transport, uint16_t port) {
  if (!transport) {
    return ESP_ERR_INVALID_ARG;
  }

  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;
  impl->port = port;
  impl->pending_response = NULL;

  ESP_LOGD(TAG, "HTTP transport initialized on port %d", port);
  return ESP_OK;
}

static esp_err_t http_start(mcp_transport_t* transport) {
  if (!transport) {
    return ESP_ERR_INVALID_ARG;
  }

  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = impl->port;
  config.max_uri_handlers = 8;
  config.max_resp_headers = 8;
  config.stack_size = 8192;

  esp_err_t ret = httpd_start(&impl->server, &config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
    return ret;
  }

  // Register POST handler for root path
  httpd_uri_t post_uri = {.uri = "/", .method = HTTP_POST, .handler = mcp_http_post_handler, .user_ctx = transport};

  ret = httpd_register_uri_handler(impl->server, &post_uri);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register POST handler: %s", esp_err_to_name(ret));
    httpd_stop(impl->server);
    impl->server = NULL;
    return ret;
  }

  // Register OPTIONS handler for CORS
  httpd_uri_t options_uri = {
      .uri = "/", .method = HTTP_OPTIONS, .handler = mcp_http_options_handler, .user_ctx = transport};

  ret = httpd_register_uri_handler(impl->server, &options_uri);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register OPTIONS handler: %s", esp_err_to_name(ret));
    // Non-fatal, continue anyway
  }

  ESP_LOGI(TAG, "HTTP server started on port %d", impl->port);
  return ESP_OK;
}

static esp_err_t http_stop(mcp_transport_t* transport) {
  if (!transport) {
    return ESP_ERR_INVALID_ARG;
  }

  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;

  if (impl->server) {
    esp_err_t ret = httpd_stop(impl->server);
    impl->server = NULL;
    ESP_LOGI(TAG, "HTTP server stopped");
    return ret;
  }

  return ESP_OK;
}

static esp_err_t http_send_response(mcp_transport_t* transport, const char* response) {
  if (!transport || !response) {
    return ESP_ERR_INVALID_ARG;
  }

  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;

  // Store response to be sent by the HTTP handler
  if (impl->pending_response) {
    free(impl->pending_response);
  }

  impl->pending_response = strdup(response);
  if (!impl->pending_response) {
    ESP_LOGE(TAG, "Failed to allocate response buffer");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

static void http_destroy(mcp_transport_t* transport) {
  if (!transport) {
    return;
  }

  mcp_http_impl_t* impl = (mcp_http_impl_t*)transport->impl_data;

  if (impl) {
    if (impl->server) {
      httpd_stop(impl->server);
    }
    if (impl->pending_response) {
      free(impl->pending_response);
    }
    free(impl);
  }

  free(transport);
  ESP_LOGI(TAG, "HTTP transport destroyed");
}

// Virtual function table
static const mcp_transport_vtable_t http_vtable = {
    .init = http_init,
    .start = http_start,
    .stop = http_stop,
    .send_response = http_send_response,
    .destroy = http_destroy,
};

mcp_transport_t* mcp_transport_http_create(void) {
  mcp_transport_t* transport = calloc(1, sizeof(mcp_transport_t));
  if (!transport) {
    ESP_LOGE(TAG, "Failed to allocate transport");
    return NULL;
  }

  mcp_http_impl_t* impl = calloc(1, sizeof(mcp_http_impl_t));
  if (!impl) {
    ESP_LOGE(TAG, "Failed to allocate HTTP implementation data");
    free(transport);
    return NULL;
  }

  transport->vtable = &http_vtable;
  transport->impl_data = impl;

  ESP_LOGI(TAG, "HTTP transport created");
  return transport;
}
