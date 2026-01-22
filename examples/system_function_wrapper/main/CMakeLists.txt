idf_component_register(SRCS "main.cpp")

target_link_libraries(${COMPONENT_LIB} INTERFACE 
  "-Wl,--wrap=esp_restart"
  "-Wl,--wrap=esp_read_mac"
)