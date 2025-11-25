idf_build_get_property(target IDF_TARGET)

set(srcs "simple_cli.cpp"
)

set(esp32_priv_requires console esp_linenoise esp_driver_uart)
set(esp32_jtag_priv_requires console esp_linenoise esp_driver_uart esp_driver_usb_serial_jtag)

# Add USB serial JTAG driver for chips that support it
if(target STREQUAL "esp32")
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "${esp32_priv_requires}")
else()
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES "${esp32_jtag_priv_requires}")
endif()
