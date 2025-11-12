#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>
#include "esp_chip_info.h"
#include "esp_system.h"

extern "C" void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("Chip model: ");
    switch(chip_info.model) {
        case CHIP_ESP32:
            printf("ESP32\n");
            break;
        case CHIP_ESP32S2:
            printf("ESP32-S2\n");
            break;
        case CHIP_ESP32S3:
            printf("ESP32-S3\n");
            break;
        case CHIP_ESP32C3:
            printf("ESP32-C3\n");
            break;
        case CHIP_ESP32C2:
            printf("ESP32-C2\n");
            break;
        case CHIP_ESP32C6:
            printf("ESP32-C6\n");
            break;
        case CHIP_ESP32H2:
            printf("ESP32-H2\n");
            break;
        case CHIP_ESP32P4:
            printf("ESP32-P4\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }

    while (true) {
        printf("This is QEMU!\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}