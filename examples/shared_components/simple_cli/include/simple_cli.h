#ifndef SIMPLE_CLI_H
#define SIMPLE_CLI_H

#include <array>
#include <string>

#include "esp_console.h"
#include "esp_linenoise.h"

/**
 * @brief User command registration
 *
 * Users define their commands using std::array and register them with the CLI:
 *
 * @code
 * #include <array>
 *
 * constexpr std::array<esp_console_cmd_t, 2> commands = {{{
 *     { .command = "restart", .help = "Restart system", .func = &restart_func },
 *     { .command = "version", .help = "Show version", .func = &version_func }
 * }}};
 *
 * SimpleCLI cli("prompt> ", SimpleCLIInterface::UART);
 * cli.register_commands(commands);
 * cli.start();
 * @endcode
 *
 * The array size is automatically deduced from the std::array template parameter.
 */

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) && \
    (defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG))
enum class SimpleCLIInterface {
  UART,
  USBSerialJTAG,
  USBFallbackToUART,
};
#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
enum class SimpleCLIInterface {
  USBSerialJTAG,
};
#elif defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
enum class SimpleCLIInterface {
  UART,
};
#else
#error "No console interface defined. Please enable at least one console interface in the project configuration."
#endif

class SimpleCLI {
 public:
  SimpleCLI(const std::string prompt, SimpleCLIInterface interface) : prompt_(prompt), interface_(interface) {}
  ~SimpleCLI() = default;

  // Register commands from a std::array - size is automatically deduced
  template <size_t N>
  void register_commands(const std::array<esp_console_cmd_t, N>& commands) {
    for (const auto& cmd : commands) {
      ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }
  }

  void register_command(const esp_console_cmd_t& command);

  void start();
  void run_repl();
  void stop();

 private:
  const std::string prompt_;
  SimpleCLIInterface interface_;

  bool cli_running_ = false;
  int cli_in_fd = -1;
  int cli_out_fd = -1;
  esp_linenoise_handle_t linenoise_handle;

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
  void configure_linenoise_uart();
#endif

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG)
  void configure_linenoise_usb();
#endif
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) && \
    (defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG))
  void configure_linenoise_usb_primary_with_uart_fallback();
#endif
};

#endif  // SIMPLE_CLI_H