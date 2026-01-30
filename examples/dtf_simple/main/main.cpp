#include <esp_event.h>
#include <stdio.h>

#include "dtf_ota.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "wifi_connect.h"

// ========= SET YOUR PRODUCT ID HERE =========
// Uncomment the following line and set your product ID to enable DTF updates
// #define DTF_PRODUCT_ID "YOUR PRODUCT ID"
// ===========================================

#ifndef DTF_PRODUCT_ID
#error "Please set DTF_PRODUCT_ID in main.cpp to enable DTF updates"
#endif

extern "C" void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  // Initialize network interface
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  printf("Current firmware version: %s\n", dtf_get_active_fw_version());
  printf("Connecting to WiFi\n");

  connect_to_wifi();

  if (is_wifi_connected()) {
    printf("Checking for updates from Deploy the Fleet\n");
    const dtf_ota_cfg_t cfg = {.product_id = DTF_PRODUCT_ID, .reboot_option = DTF_NO_REBOOT};
    dtf_get_firmware_update(&cfg);
  }
  else {
    printf("WiFi not connected, skipping DTF update check\n");
  }
}