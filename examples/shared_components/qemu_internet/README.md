# qemu_internet Component

A reusable ESP-IDF component that enables internet access for ESP32 projects running inside QEMU. This component provides a simple API to initialize and configure Ethernet networking in the QEMU environment, allowing your ESP32 application to access external network resources.

## Overview

When running ESP32 applications in QEMU, standard WiFi networking is not available. Instead, QEMU provides an OpenEth MAC device that can be used to access the internet via the host system's network. This component abstracts the configuration and initialization of this Ethernet interface, making it easy to add internet connectivity even if you 
need to run your project in QEMU for testing or debugging.

## Features

- Simple two-function API for connecting and disconnecting
- Automatic DHCP configuration
- Blocks until IP address is obtained
- Compatible with all standard ESP-IDF networking components (HTTP client, MQTT, etc.)

## How It Works

The component initializes the OpenEth MAC and PHY drivers specifically for QEMU environments. It:

1. Creates a network interface with default Ethernet configuration
2. Configures DHCP client for automatic IP assignment
3. Registers event handlers for IP acquisition
4. Starts the Ethernet driver
5. Waits for an IP address to be assigned before returning

Once connected, your application can use any ESP-IDF networking APIs (HTTP client, sockets, DNS, etc.) as if running on physical hardware.

## Integration

### 1. Add Required sdkconfig Settings

The following Kconfig options must be enabled in your `sdkconfig.defaults` file:

```
CONFIG_ETH_ENABLED=y
CONFIG_ETH_USE_ESP32_EMAC=y
CONFIG_ETH_USE_OPENETH=y
```

Additionally, when using HTTPS or TLS, you must disable hardware cryptography acceleration (causes crashes in QEMU):

```
CONFIG_MBEDTLS_HARDWARE_AES=n
CONFIG_MBEDTLS_HARDWARE_SHA=n
CONFIG_MBEDTLS_HARDWARE_MPI=n
```

### 3. Add `ethernet_init` Component to Your Project

This is required for the `qemu_internet` component to work and, due to currently limitations in the IDF build system, 
must be included at the project level. Add the dependency by running the following from the root of your project:

```sh
idf.py add-dependency "espressif/ethernet_init"
```

### 2. Add Component to Your Project

The `qemu_internet` component can be included as a shared component. Add it to your main component's `CMakeLists.txt`:

```cmake
idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES qemu_internet)
```

Also add it to your **project-level** idf_component.yml

```yml
dependencies:
  qemu_internet:
    path: ../../shared_components/qemu_internet
  espressif/ethernet_init: '*'
```

### 3. Include the Header

In your source code:

```c
#include "qemu_internet.h"
```

### 4. Initialize Networking in app_main()

Before calling `qemu_internet_connect()`, you must initialize the TCP/IP stack and event loop:

```c
void app_main(void) {
    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop for handling network events
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Connect to internet via QEMU Ethernet
    esp_err_t ret = qemu_internet_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        return;
    }

    // Network is now available - use HTTP client, sockets, etc.
    // ...
}
```

### 5. (Optional) Disconnect When Done

If your application needs to cleanly shut down the network interface:

```c
qemu_internet_disconnect();
```

## API Reference

### `esp_err_t qemu_internet_connect(void)`

Initializes the Ethernet interface and waits for DHCP to assign an IP address.

**Returns:**
- `ESP_OK` on success
- `ESP_ERR_NO_MEM` if memory allocation fails
- Other error codes from underlying ESP-IDF Ethernet driver

**Note:** This function blocks until an IP address is obtained. Typically takes 1-3 seconds in QEMU.

### `void qemu_internet_disconnect(void)`

Tears down the Ethernet connection and releases all associated resources.

Call this function when you no longer need network access or before restarting the network interface.

## Example Usage

```c
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "qemu_internet.h"

static const char* TAG = "example";

void app_main(void) {
    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Connect to internet
    ESP_LOGI(TAG, "Connecting to internet...");
    ESP_ERROR_CHECK(qemu_internet_connect());
    ESP_LOGI(TAG, "Connected!");

    // Now you can use any networking APIs
    esp_http_client_config_t config = {
        .url = "http://example.com",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    // Cleanup when done
    qemu_internet_disconnect();
}
```

## Running in QEMU

To run your project with internet access in QEMU:

```bash
idf.py qemu --gdb monitor
```

QEMU's network backend will bridge the Ethernet interface to your host system's network, allowing full internet access.

## Troubleshooting

### No IP Address Obtained

- Ensure QEMU is properly configured with network support
- Check that your host system has a working network connection
- Verify the required Kconfig options are set in `sdkconfig.defaults`

### HTTPS/TLS Crashes

- Make sure hardware crypto acceleration is disabled (see Kconfig settings above)
- QEMU does not emulate the ESP32's cryptographic hardware very well

### Build Errors

- Ensure the `qemu_internet` component is in your component search path
- Verify all dependencies are available (`esp_netif`, `esp_eth`, `ethernet_init`)

## Dependencies

This component requires:
- `esp_netif` - ESP-IDF network interface abstraction
- `esp_eth` - Ethernet driver
- `ethernet_init` - Ethernet initialization helpers
- `nvs_flash` - Non-volatile storage (used by networking stack)

## License

This component is based on Espressif's Ethernet examples and follows the same licensing.
