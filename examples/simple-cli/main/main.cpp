#include <stdio.h>
#include <string.h>

#include <array>
#include <chrono>
#include <thread>

#include "esp_log.h"
#include "esp_mac.h"
#include "simple_cli.h"

static int restart(int argc, char** argv) {
  ESP_LOGI("COMMAND", "Restarting system...");
  esp_restart();
  return 0;
}

static int get_id(int argc, char** argv) {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  printf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return 0;
}

constexpr std::array<esp_console_cmd_t, 2> commands = {{{
                                                            .command = "restart",
                                                            .help = "Restart the system",
                                                            .hint = NULL,
                                                            .func = &restart,
                                                            .argtable = NULL,
                                                            .func_w_context = NULL,
                                                            .context = NULL,
                                                        },
                                                        {
                                                            .command = "id",
                                                            .help = "Get the device ID",
                                                            .hint = NULL,
                                                            .func = &get_id,
                                                            .argtable = NULL,
                                                            .func_w_context = NULL,
                                                            .context = NULL,
                                                        }}};

extern "C" void app_main(void) {
  SimpleCLI cli("simple-cli>", SimpleCLIInterface::UART);
  cli.register_commands(commands);
  cli.start();
  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}