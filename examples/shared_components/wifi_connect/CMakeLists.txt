idf_build_get_property(target IDF_TARGET)

set(srcs "wifi_connect.c"
)



idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_netif esp_wifi nvs_flash)
