idf_component_register(SRCS "main.cpp"
                            "rest_server.c"
                    PRIV_REQUIRES esp_http_server esp_driver_gpio fatfs json spiffs nvs_flash app_update esp_timer
                    INCLUDE_DIRS ".")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/info-app")
set(WEB_DIST_DIR "${WEB_SRC_DIR}/dist")

# Check if npm is available
find_program(NPM_EXECUTABLE npm)

if(NPM_EXECUTABLE)
    # Add custom command to build Vue app
    add_custom_command(
        OUTPUT ${WEB_DIST_DIR}/index.html
        COMMAND ${NPM_EXECUTABLE} run build
        WORKING_DIRECTORY ${WEB_SRC_DIR}
        COMMENT "Building Vue app..."
        DEPENDS ${WEB_SRC_DIR}/src/App.vue 
                ${WEB_SRC_DIR}/src/components/MemoryChart.vue
                ${WEB_SRC_DIR}/package.json
    )
    
    # Create a custom target that depends on the Vue build
    add_custom_target(vue_build ALL
        DEPENDS ${WEB_DIST_DIR}/index.html
    )
    
    # Create the LittleFS partition and make it depend on vue_build
    littlefs_create_partition_image(www ${WEB_DIST_DIR} FLASH_IN_PROJECT DEPENDS vue_build)
    
    message(STATUS "Vue app will be automatically built during firmware build")
else()
    # Fallback: check if dist exists
    if(EXISTS ${WEB_DIST_DIR})
        littlefs_create_partition_image(www ${WEB_DIST_DIR} FLASH_IN_PROJECT)
        message(WARNING "npm not found. Using existing dist folder. Install Node.js/npm for automatic builds.")
    else()
        message(FATAL_ERROR "${WEB_DIST_DIR} doesn't exist and npm is not available. Please install Node.js/npm or manually run 'npm run build' in ${WEB_SRC_DIR}")
    endif()
endif()

