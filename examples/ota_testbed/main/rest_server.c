/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <fcntl.h>
#include <string.h>

#include "cJSON.h"
#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_vfs.h"

static const char* REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                        \
  do {                                                                           \
    if (!(a)) {                                                                  \
      ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      goto goto_tag;                                                             \
    }                                                                            \
  } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
  char base_path[ESP_VFS_PATH_MAX + 1];
  char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t* req, const char* filepath) {
  const char* type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  }
  else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "text/xml";
  }
  return httpd_resp_set_type(req, type);
}

static const char* esp_reset_reason_to_name(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN:
      return "unknown";
    case ESP_RST_POWERON:
      return "poweron";
    case ESP_RST_EXT:
      return "ext";
    case ESP_RST_SW:
      return "sw";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "int_wdt";
    case ESP_RST_TASK_WDT:
      return "task_wdt";
    case ESP_RST_WDT:
      return "wdt";
    case ESP_RST_DEEPSLEEP:
      return "deepsleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    case ESP_RST_USB:
      return "usb";
    case ESP_RST_JTAG:
      return "jtag";
    case ESP_RST_EFUSE:
      return "efuse";
    case ESP_RST_PWR_GLITCH:
      return "pwr_glitch";
    case ESP_RST_CPU_LOCKUP:
      return "cpu_lockup";
    default:
      return "unknown";
  }
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t* req) {
  char filepath[FILE_PATH_MAX];

  rest_server_context_t* rest_context = (rest_server_context_t*)req->user_ctx;
  strlcpy(filepath, rest_context->base_path, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  }
  else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
    return ESP_FAIL;
  }

  set_content_type_from_file(req, filepath);

  char* chunk = rest_context->scratch;
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
    }
    else if (read_bytes > 0) {
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(REST_TAG, "File sending failed!");
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(REST_TAG, "File sending complete");
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  cJSON* root = cJSON_CreateObject();
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  const esp_app_desc_t* app_desc = esp_app_get_description();

  cJSON_AddStringToObject(root, "version", IDF_VER);
  cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  cJSON_AddStringToObject(root, "app_version", app_desc->version);
  cJSON_AddStringToObject(root, "build_date", __DATE__);
  cJSON_AddStringToObject(root, "build_time", __TIME__);

  // In the updated version the info endpoint returns additional information
  // - uptime in milliseconds
  // - restart reason as reason: {main: "...", hint: "..."}
  uint64_t uptime_ms = esp_timer_get_time() / 1000;
  cJSON_AddNumberToObject(root, "uptime_ms", uptime_ms);

  esp_reset_reason_t reset_reason = esp_reset_reason();
  cJSON_AddStringToObject(root, "reset_reason", esp_reset_reason_to_name(reset_reason));

  const char* sys_info = cJSON_Print(root);
  httpd_resp_sendstr(req, sys_info);
  free((void*)sys_info);
  cJSON_Delete(root);
  return ESP_OK;
}

/* Simple handler for getting memory data */
static esp_err_t system_memory_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  cJSON* root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
  cJSON_AddNumberToObject(root, "min_free_heap", esp_get_minimum_free_heap_size());
  const char* sys_info = cJSON_Print(root);
  httpd_resp_sendstr(req, sys_info);
  free((void*)sys_info);
  cJSON_Delete(root);
  return ESP_OK;
}

/* Handler for intentionally leaking memory */
static esp_err_t leak_memory_post_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");

  // Leak random amount between 1024 and 10240 bytes (10kB)
  size_t leak_size = 1024 + (esp_random() % 9217);
  void* leaked_memory = malloc(leak_size);

  cJSON* root = cJSON_CreateObject();
  if (leaked_memory) {
    cJSON_AddNumberToObject(root, "leaked_bytes", leak_size);
    cJSON_AddStringToObject(root, "status", "success");
    ESP_LOGW(REST_TAG, "Intentionally leaked %zu bytes at %p", leak_size, leaked_memory);
  }
  else {
    cJSON_AddNumberToObject(root, "leaked_bytes", 0);
    cJSON_AddStringToObject(root, "status", "failed");
    ESP_LOGE(REST_TAG, "Failed to allocate %zu bytes for leak", leak_size);
  }

  const char* response = cJSON_Print(root);
  httpd_resp_sendstr(req, response);
  free((void*)response);
  cJSON_Delete(root);
  return ESP_OK;
}

/* Handler for restarting the ESP32 */
static esp_err_t restart_post_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"restarting\"}");
  ESP_LOGW(REST_TAG, "Restarting ESP32...");
  vTaskDelay(pdMS_TO_TICKS(100));  // Give time for response to send
  esp_restart();
  return ESP_OK;
}

/* Handler for crashing the ESP32 */
static esp_err_t crash_post_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"status\":\"crashing\"}");
  ESP_LOGE(REST_TAG, "Intentionally crashing ESP32...");
  vTaskDelay(pdMS_TO_TICKS(100));  // Give time for response to send
  abort();
  return ESP_OK;
}

esp_err_t start_rest_server(const char* base_path) {
  REST_CHECK(base_path, "wrong base path", err);
  rest_server_context_t* rest_context = calloc(1, sizeof(rest_server_context_t));
  REST_CHECK(rest_context, "No memory for rest context", err);
  strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;

  ESP_LOGI(REST_TAG, "Starting HTTP Server");
  REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

  /* URI handler for fetching system info */
  httpd_uri_t system_info_get_uri = {
      .uri = "/api/v1/system/info", .method = HTTP_GET, .handler = system_info_get_handler, .user_ctx = rest_context};
  httpd_register_uri_handler(server, &system_info_get_uri);

  /* URI handler for fetching memory data */
  httpd_uri_t system_memory_get_uri = {.uri = "/api/v1/system/memory",
                                       .method = HTTP_GET,
                                       .handler = system_memory_get_handler,
                                       .user_ctx = rest_context};
  httpd_register_uri_handler(server, &system_memory_get_uri);

  /* URI handler for leaking memory */
  httpd_uri_t leak_memory_post_uri = {
      .uri = "/api/v1/system/leak", .method = HTTP_POST, .handler = leak_memory_post_handler, .user_ctx = rest_context};
  httpd_register_uri_handler(server, &leak_memory_post_uri);

  /* URI handler for restarting */
  httpd_uri_t restart_post_uri = {
      .uri = "/api/v1/system/restart", .method = HTTP_POST, .handler = restart_post_handler, .user_ctx = rest_context};
  httpd_register_uri_handler(server, &restart_post_uri);

  /* URI handler for crashing */
  httpd_uri_t crash_post_uri = {
      .uri = "/api/v1/system/crash", .method = HTTP_POST, .handler = crash_post_handler, .user_ctx = rest_context};
  httpd_register_uri_handler(server, &crash_post_uri);

  /* URI handler for getting web server files */
  httpd_uri_t common_get_uri = {
      .uri = "/*", .method = HTTP_GET, .handler = rest_common_get_handler, .user_ctx = rest_context};
  httpd_register_uri_handler(server, &common_get_uri);

  return ESP_OK;
err_start:
  free(rest_context);
err:
  return ESP_FAIL;
}
