#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

extern "C" void app_main(void) {
  while (true) {
    printf("Hello, Minimal Buil!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}