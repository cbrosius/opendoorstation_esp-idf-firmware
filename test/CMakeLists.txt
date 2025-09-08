idf_component_register(SRCS "test_main.c" "test_config_manager.c" "test_config_storage.c" "test_config_env.c" "test_io_manager.c" "test_io_events.c" "test_io_integration.c" "mocks/mock_nvs.c" "mocks/mock_gpio.c"
                    INCLUDE_DIRS "." "mocks"
                    REQUIRES unity main nvs_flash driver esp_event esp_timer)