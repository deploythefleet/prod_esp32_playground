#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "esp_netif.h"
#include "nvs_flash.h"
#include "wifi_connect.h"

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

  connect_to_wifi();

  while (true) {
    if (is_wifi_connected()) {
      printf("WiFi is connected!\n");
    }
    else {
      printf("WiFi is not connected.\n");
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}