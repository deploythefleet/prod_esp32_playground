#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>

extern "C" void app_main(void)
{
    while (true) {
        printf("Hello, Minimal Build!\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}