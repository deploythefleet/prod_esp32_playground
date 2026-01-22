#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Since this is a C++ project you need to declare the wrapper functions with C linkage
extern "C" {
void __real_esp_restart(void);
esp_err_t __real_esp_read_mac(uint8_t* mac, esp_mac_type_t type);

void __wrap_esp_restart(void) {
  // You can use C++ features inside the wrapper
  // as long as the function signature is C-compatible
  // **************************
  // ADD YOUR CUSTOM LOGIC HERE
  // **************************
  ESP_LOGW("app_main", "I refuse to restart the system!");
  while (true) {
    ESP_LOGE("app_main", "Ah, ah, ah. You didn't say the magic word!");
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  // Uncomment this call to wrap the original function. Comment it out to replace the functionality entirely
  // with your custom logic above.
  // __real_esp_restart();
}

esp_err_t __wrap_esp_read_mac(uint8_t* mac, esp_mac_type_t type) {
  // Custom MAC address
  const uint8_t custom_mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
  memcpy(mac, custom_mac, 6);
  ESP_LOGI("app_main", "Providing custom MAC address");
  return ESP_OK;
}
}  // extern "C"

void print_device_id() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  ESP_LOGI("DeviceID", "Device MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
           mac[5]);
}

extern "C" void app_main(void) {
  while (true) {
    // print stack low water mark
    ESP_LOGI("app_main", "Stack high water mark: %u bytes", uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI("app_main", "Free heap size: %u bytes", esp_get_free_heap_size());
    print_device_id();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    esp_restart();
  }
}