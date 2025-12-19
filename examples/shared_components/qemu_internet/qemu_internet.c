/*
 * SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>

// #include "driver/gpio.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "qemu_internet.h"

#if CONFIG_ETH_USE_OPENETH
#include "esp_eth_mac_openeth.h"
#include "esp_eth_phy.h"
#else
#include "ethernet_init.h"
#endif

static const char* TAG = "qemu_internet";
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;

static esp_netif_t* eth_start(void);
static void eth_stop(void);

/** Event handler for Ethernet events */

static void eth_on_got_ip(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  // if (!example_is_our_netif("eth0", event->esp_netif)) {
  //   return;
  // }
  ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif),
           IP2STR(&event->ip_info.ip));
  xSemaphoreGive(s_semph_get_ip_addrs);
}

static esp_eth_handle_t s_eth_handle = NULL;
static esp_eth_mac_t* s_mac = NULL;
static esp_eth_phy_t* s_phy = NULL;
static esp_eth_netif_glue_handle_t s_eth_glue = NULL;
static esp_netif_t* s_eth_netif = NULL;

#if !CONFIG_ETH_USE_OPENETH
static esp_eth_handle_t* s_eth_handles = NULL;
static uint8_t s_eth_count = 0;
#endif

static esp_netif_t* eth_start(void) {
  // esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_inherent_config_t esp_netif_config;
  esp_netif_config.flags =
      (esp_netif_flags_t)(ESP_NETIF_IPV4_ONLY_FLAGS(ESP_NETIF_DHCP_CLIENT) | ESP_NETIF_DEFAULT_ARP_FLAGS |
                          ESP_NETIF_FLAG_EVENT_IP_MODIFIED | ESP_NETIF_DEFAULT_IPV6_AUTOCONFIG_FLAGS),
  esp_netif_config.get_ip_event = IP_EVENT_ETH_GOT_IP;
  esp_netif_config.lost_ip_event = IP_EVENT_ETH_LOST_IP;
  esp_netif_config.if_key = "ETH_DEF";
  esp_netif_config.route_prio = 50;
  esp_netif_config.bridge_info = NULL;

  // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
  esp_netif_config.if_desc = "eth0";
  esp_netif_config.route_prio = 64;
  esp_netif_config_t netif_config = {.base = &esp_netif_config, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
  s_eth_netif = esp_netif_new(&netif_config);

#if CONFIG_ETH_USE_OPENETH
  // Initialize OpenEth MAC and PHY for QEMU
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.autonego_timeout_ms = 100;

  s_mac = esp_eth_mac_new_openeth(&mac_config);
  s_phy = esp_eth_phy_new_dp83848(&phy_config);

  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(s_mac, s_phy);
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &s_eth_handle));

  s_eth_glue = esp_eth_new_netif_glue(s_eth_handle);
#else
  // Initialize Ethernet driver using ethernet_init component
  ESP_ERROR_CHECK(ethernet_init_all(&s_eth_handles, &s_eth_count));

  if (s_eth_handles == NULL || s_eth_count == 0) {
    ESP_LOGE(TAG, "No Ethernet device initialized");
    return NULL;
  }

  s_eth_handle = s_eth_handles[0];
  s_eth_glue = esp_eth_new_netif_glue(s_eth_handle);
#endif

  esp_netif_attach(s_eth_netif, s_eth_glue);

  // Register user defined event handlers
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &eth_on_got_ip, NULL));

  ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));

  return s_eth_netif;
}

static void eth_stop(void) {
  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, &eth_on_got_ip));
  ESP_ERROR_CHECK(esp_eth_stop(s_eth_handle));
  ESP_ERROR_CHECK(esp_eth_del_netif_glue(s_eth_glue));
  esp_netif_destroy(s_eth_netif);

#if CONFIG_ETH_USE_OPENETH
  ESP_ERROR_CHECK(esp_eth_driver_uninstall(s_eth_handle));
  ESP_ERROR_CHECK(s_phy->del(s_phy));
  ESP_ERROR_CHECK(s_mac->del(s_mac));
  s_mac = NULL;
  s_phy = NULL;
#else
  ethernet_deinit_all(s_eth_handles);
  s_eth_handles = NULL;
  s_eth_count = 0;
#endif

  s_eth_glue = NULL;
  s_eth_netif = NULL;
  s_eth_handle = NULL;
}

/* tear down connection, release resources */
void qemu_internet_disconnect(void) {
  if (s_semph_get_ip_addrs == NULL) {
    return;
  }
  vSemaphoreDelete(s_semph_get_ip_addrs);
  s_semph_get_ip_addrs = NULL;
  eth_stop();
}

esp_err_t qemu_internet_connect(void) {
  s_semph_get_ip_addrs = xSemaphoreCreateBinary();
  if (s_semph_get_ip_addrs == NULL) {
    return ESP_ERR_NO_MEM;
  }
  eth_start();
  ESP_LOGI(TAG, "Waiting for IP(s).");
  xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
  return ESP_OK;
}