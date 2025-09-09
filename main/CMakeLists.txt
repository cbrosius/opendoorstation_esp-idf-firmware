# Determine if we need test component
set(MAIN_REQUIRES nvs_flash driver esp_event esp_timer esp_http_server spiffs json esp_wifi unity)
set(MAIN_PRIV_REQUIRES "")

# Add test component if test mode is enabled
if(DEFINED ENV{RUN_TESTS})
    list(APPEND MAIN_PRIV_REQUIRES test)
    message(STATUS "Test mode enabled - adding test component to build")
endif()

idf_component_register(SRCS "app_main.c" "config_manager.c" "io_manager.c" "io_events.c" "sip_manager.c" "sip_io_integration.c" "esp_sip.c" "web_server.c" "app_controller.c" "error_handler.c" "wifi_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${MAIN_REQUIRES}
                    PRIV_REQUIRES ${MAIN_PRIV_REQUIRES})

# Pass environment variables as compile definitions
if(DEFINED ENV{WIFI_SSID})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_WIFI_SSID="$ENV{WIFI_SSID}")
endif()

if(DEFINED ENV{WIFI_PASSWORD})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_WIFI_PASSWORD="$ENV{WIFI_PASSWORD}")
endif()

if(DEFINED ENV{SIP_USER})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_SIP_USER="$ENV{SIP_USER}")
endif()

if(DEFINED ENV{SIP_DOMAIN})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_SIP_DOMAIN="$ENV{SIP_DOMAIN}")
endif()

if(DEFINED ENV{SIP_PASSWORD})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_SIP_PASSWORD="$ENV{SIP_PASSWORD}")
endif()

if(DEFINED ENV{SIP_CALLEE})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_SIP_CALLEE="$ENV{SIP_CALLEE}")
endif()

if(DEFINED ENV{WEB_PORT})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_WEB_PORT=$ENV{WEB_PORT})
endif()

if(DEFINED ENV{DOOR_PULSE_DURATION})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_DOOR_PULSE_DURATION=$ENV{DOOR_PULSE_DURATION})
endif()

# Enable test mode if environment variable is set
if(DEFINED ENV{RUN_TESTS})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CONFIG_RUN_TESTS=1)
    message(STATUS "Test mode compile definition added")
endif()