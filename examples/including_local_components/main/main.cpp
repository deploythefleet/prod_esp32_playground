#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "wifi_connect.h"

extern "C" void app_main(void) {
  connect_to_wifi();

  while (true) {
    if (is_wifi_connected()) {
      printf("WiFi is connected!\n");
    } else {
      printf("WiFi is not connected.\n");
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}