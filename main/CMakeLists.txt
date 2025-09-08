idf_component_register(SRCS "app_main.c" "config_manager.c" "io_manager.c" "io_events.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash driver esp_event esp_timer)

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