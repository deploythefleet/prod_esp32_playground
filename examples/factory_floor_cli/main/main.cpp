#include <stdio.h>
#include <string.h>

#include <chrono>
#include <thread>

#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_console.h"
#include "esp_linenoise.h"
#include "esp_log.h"
#include "esp_mac.h"

static char model_number[64] = {0};

static int restart(int argc, char** argv) { esp_restart(); }

// Command is model get|set <model_number>
static int model_number_cmd(int argc, char** argv) {
  if (argc == 2 && strcmp(argv[1], "get") == 0) {
    printf("Model Number: %s\n", model_number);
  } else if (argc == 3 && strcmp(argv[1], "set") == 0) {
    strncpy(model_number, argv[2], sizeof(model_number) - 1);
    model_number[sizeof(model_number) - 1] = '\0';
    printf("Model Number set to: %s\n", model_number);
  } else {
    printf("Usage: model get|set <model_number>\n");
  }
  return 0;
}

static int get_id_cmd(int argc, char** argv) {
  // use device mac address as unique ID
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return 0;
}

static void register_commands() {
  const esp_console_cmd_t restart_cmd = {
      .command = "restart",
      .help = "Restart the system",
      .hint = NULL,
      .func = &restart,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&restart_cmd));

  const esp_console_cmd_t serial_cmd = {
      .command = "model",
      .help = "Get or set the model number",
      .hint = "<get|set> [model_number]",
      .func = &model_number_cmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&serial_cmd));

  const esp_console_cmd_t id_cmd = {
      .command = "id",
      .help = "Get device ID (MAC address)",
      .hint = NULL,
      .func = &get_id_cmd,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&id_cmd));
}

extern "C" void app_main(void) {
  esp_linenoise_handle_t linenoise_handle;
  esp_linenoise_config_t config;

  uart_vfs_dev_port_set_rx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  uart_vfs_dev_port_set_tx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_CRLF);

  /* Configure UART. Note that REF_TICK is used so that the baud rate remains
   * correct while APB frequency is changing in light sleep mode.
   */
  const uart_config_t uart_config = {.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
                                     .data_bits = UART_DATA_8_BITS,
                                     .parity = UART_PARITY_DISABLE,
                                     .stop_bits = UART_STOP_BITS_1,
                                     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                     .rx_flow_ctrl_thresh = 0,
#if SOC_UART_SUPPORT_REF_TICK
                                     .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
                                     .source_clk = UART_SCLK_XTAL,
#endif
                                     .flags = {
                                         .allow_pd = 0,
                                         .backup_before_sleep = 0,
                                     }};
  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(uart_driver_install((uart_port_t)UART_NUM_0, 256, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config((uart_port_t)UART_NUM_0, &uart_config));

  /* Tell VFS to use UART driver */
  uart_vfs_dev_use_driver(UART_NUM_0);

  esp_linenoise_get_instance_config_default(&config);
  config.prompt = "widget>";
  config.allow_empty_line = false;
  config.in_fd = fileno(stdin);
  config.out_fd = fileno(stdout);
  config.allow_dumb_mode = true;
  esp_linenoise_create_instance(&config, &linenoise_handle);

  /* Initialize the console */
  esp_console_config_t console_config = {
      .max_cmdline_length = 256, .max_cmdline_args = 8, .heap_alloc_caps = 0, .hint_bold = 0};
  ESP_ERROR_CHECK(esp_console_init(&console_config));

  /* Register commands */
  esp_console_register_help_command();
  register_commands();

  char buffer[128];

  while (true) {
    memset(buffer, 0, sizeof(buffer));
    esp_err_t err = esp_linenoise_get_line(linenoise_handle, buffer, sizeof(buffer));
    if (err != ESP_OK) { /* Continue on EOF or error */
      continue;
    }

    int ret = 0;
    if (strlen(buffer) != 0) {
      esp_err_t err = esp_console_run(buffer, &ret);
      if (err == ESP_ERR_NOT_FOUND) {
        printf("Unrecognized command\n");
      } else if (err == ESP_ERR_INVALID_ARG) {
        // command was empty
      } else if (err == ESP_OK && ret != ESP_OK) {
        printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
      } else if (err != ESP_OK) {
        printf("Internal error: %s\n", esp_err_to_name(err));
      }
    }
  }
}