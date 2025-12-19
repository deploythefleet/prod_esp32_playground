idf_build_get_property(target IDF_TARGET)

set(srcs "qemu_internet.c"
)

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_netif nvs_flash esp_eth ethernet_init)
