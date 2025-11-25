
#include "simple_cli.h"

#include <fcntl.h>
#include <string.h>

#include <array>

#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "freertos/task.h"

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#endif

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#endif

constexpr const char* TAG = "SimpleCLI";

/**
 * Overall the architecture consists of a few parts. First, the console itself is a singleton
 * in the case of the IDF component. It knows about commands and how to execute them.
 * This is separate from the input/output interface, which is handled by linenoise in this case.
 * The simple CLI component is a thin wrapper around these pieces to make it easier to set up
 * a CLI with different interfaces (UART, USB Serial JTAG, or both).
 */

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) && \
    (defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG))
void SimpleCLI::configure_linenoise_usb_primary_with_uart_fallback() {
  // Set up the console to use the USB Serial port if a device is connected.
  // Otherwise, it will default to UART for the input/output interface.
  if (usb_serial_jtag_is_connected()) {
    configure_linenoise_usb();
  }
  else {
    configure_linenoise_uart();
  }
}
#endif

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
void SimpleCLI::configure_linenoise_uart() {
  ESP_LOGI(TAG, "Using UART0 for CLI interface");

  // Use UART for CLI interface
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

  cli_in_fd = fileno(stdin);
  cli_out_fd = fileno(stdout);
}
#endif

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
void SimpleCLI::configure_linenoise_usb() {
  ESP_LOGI(TAG, "Using USB Serial JTAG for CLI interface");

  usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  // /* Move the caret to the beginning of the next line on '\n' */
  usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  usb_serial_jtag_driver_config_t jtag_config = {
      .tx_buffer_size = 256,
      .rx_buffer_size = 256,
  };

  // /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&jtag_config));
  ESP_ERROR_CHECK(usb_serial_jtag_vfs_register());

  // /* Tell vfs to use usb-serial-jtag driver */
  usb_serial_jtag_vfs_use_driver();

  cli_in_fd = open("/dev/usbserjtag", O_RDONLY | O_NONBLOCK);
  cli_out_fd = open("/dev/usbserjtag", O_WRONLY);
}
#endif

static void cli_thread(void* params) {
  SimpleCLI* cli = static_cast<SimpleCLI*>(params);
  cli->run_repl();
}

void SimpleCLI::start() {
  // 1. Set up the console configuration
  esp_console_config_t console_config = {
      .max_cmdline_length = 256, .max_cmdline_args = 8, .heap_alloc_caps = 0, .hint_color = 39, .hint_bold = 0};
  ESP_ERROR_CHECK(esp_console_init(&console_config));

  // Register the help command by default
  esp_console_register_help_command();

  // 2. Set up the linenoise configuration
  switch (interface_) {
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
    case SimpleCLIInterface::UART:
      configure_linenoise_uart();
      break;
#endif
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
    case SimpleCLIInterface::USBSerialJTAG:
      configure_linenoise_usb();
      break;
#endif
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) && \
    (defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG))
    case SimpleCLIInterface::USBFallbackToUART:
      configure_linenoise_usb_primary_with_uart_fallback();
      break;
#endif
    default:
      ESP_LOGE(TAG, "Invalid CLI interface selected");
      return;
  }
  ESP_LOGD(TAG, "CLI STDIN FD: %d, CLI STDOUT FD: %d", cli_in_fd, cli_out_fd);

  // 3. Start the CLI loop on a separate thread
  cli_running_ = true;
  xTaskCreate(cli_thread, "cli_thread", 4096, this, tskIDLE_PRIORITY + 5, NULL);
}

void SimpleCLI::stop() { cli_running_ = false; }

void SimpleCLI::run_repl() {
  esp_linenoise_config_t config;
  esp_linenoise_get_instance_config_default(&config);
  config.prompt = prompt_.c_str();
  config.allow_empty_line = false;
  config.in_fd = cli_in_fd;
  config.out_fd = cli_out_fd;
  config.allow_dumb_mode = true;
  esp_linenoise_create_instance(&config, &linenoise_handle);

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
      }
      else if (err == ESP_ERR_INVALID_ARG) {
        // command was empty
      }
      else if (err == ESP_OK && ret != ESP_OK) {
        printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
      }
      else if (err != ESP_OK) {
        printf("Internal error: %s\n", esp_err_to_name(err));
      }
    }
  }
}

void SimpleCLI::register_command(const esp_console_cmd_t& command) {
  ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}