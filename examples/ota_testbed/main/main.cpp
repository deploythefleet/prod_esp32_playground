#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "esp_event.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "wifi_connect.h"

constexpr const char* MDNS_INSTANCE = "esp home web server";
constexpr const char* TAG = "ota_testbed";

extern "C" {
esp_err_t start_rest_server(const char* base_path);
}

static void initialise_mdns(void) {
  mdns_init();
  mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
  mdns_instance_name_set(MDNS_INSTANCE);

  mdns_txt_item_t serviceTxtData[] = {{"board", "esp32"}, {"path", "/"}};

  ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                   sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

static void init_fs() {
  ESP_LOGI(TAG, "Initializing LittleFS");

  esp_vfs_littlefs_conf_t storage_conf = {
      .base_path = "/storage",
      .partition_label = "storage",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  esp_vfs_littlefs_conf_t www_conf = {
      .base_path = "/www",
      .partition_label = "www",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  // Use settings defined above to initialize and mount LittleFS filesystem.
  // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_littlefs_register(&storage_conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format storage filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find LittleFS storage partition");
    }
    else {
      ESP_LOGE(TAG, "Failed to initialize storage LittleFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  ret = esp_vfs_littlefs_register(&www_conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format www filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find LittleFS www partition");
    }
    else {
      ESP_LOGE(TAG, "Failed to initialize www LittleFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(storage_conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get LittleFS storage partition information (%s)", esp_err_to_name(ret));
    esp_littlefs_format(storage_conf.partition_label);
  }
  else {
    ESP_LOGI(TAG, "Storage partition size: total: %d, used: %d", total, used);
  }

  ret = esp_littlefs_info(www_conf.partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get LittleFS www partition information (%s)", esp_err_to_name(ret));
    esp_littlefs_format(www_conf.partition_label);
  }
  else {
    ESP_LOGI(TAG, "www partition size: total: %d, used: %d", total, used);
  }
}

extern "C" void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  initialise_mdns();
  netbiosns_init();
  netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);
  init_fs();
  connect_to_wifi();
  start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT);
}