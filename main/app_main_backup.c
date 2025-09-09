#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "config_manager.h"
#include "io_manager.h"
#include "io_events.h"
#include "sip_manager.h"
#include "app_controller.h"
#include "error_handler.h"

static const char *TAG = "sip_door_station";

// Event handlers
static void button_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS) {
        switch (event_id) {
            case IO_EVENT_BUTTON_PRESSED: {
                io_button_event_data_t* data = (io_button_event_data_t*)event_data;
                ESP_LOGI(TAG, "Button pressed at timestamp: %" PRIu32, data->timestamp);
                
                // Notify application controller of button press
                esp_err_t result = app_controller_handle_button_press();
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to handle button press: %s", esp_err_to_name(result));
                }
                break;
            }
            case IO_EVENT_BUTTON_RELEASED: {
                io_button_event_data_t* data = (io_button_event_data_t*)event_data;
                ESP_LOGI(TAG, "Button released at timestamp: %" PRIu32, data->timestamp);
                break;
            }
        }
    }
}

static void relay_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS && event_id == IO_EVENT_RELAY_STATE_CHANGED) {
        io_relay_event_data_t* data = (io_relay_event_data_t*)event_data;
        ESP_LOGI(TAG, "Relay %d changed from %s to %s at timestamp: %" PRIu32,
                 data->relay,
                 data->old_state == RELAY_STATE_ON ? "ON" : "OFF",
                 data->new_state == RELAY_STATE_ON ? "ON" : "OFF",
                 data->timestamp);
        
        // Notify application controller of relay state change
        esp_err_t result = app_controller_handle_relay_state_change(data->relay, data->new_state);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to handle relay state change: %s", esp_err_to_name(result));
        }
    }
}

// SIP event handler
static void sip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == ESP_EVENT_ANY_BASE) { // SIP events will be handled through SIP manager callbacks
        sip_event_data_t* data = (sip_event_data_t*)event_data;
        
        switch (data->event_type) {
            case SIP_EVENT_REGISTERED:
                ESP_LOGI(TAG, "SIP registered successfully");
                app_controller_handle_sip_state_change(SIP_STATE_REGISTERED);
                break;
                
            case SIP_EVENT_REGISTRATION_FAILED:
                ESP_LOGE(TAG, "SIP registration failed: %s", data->data.error.error_message);
                app_controller_handle_sip_state_change(SIP_STATE_ERROR);
                app_controller_handle_error(data->data.error.error_code, data->data.error.error_message);
                break;
                
            case SIP_EVENT_CALL_STARTED:
                ESP_LOGI(TAG, "SIP call started");
                app_controller_handle_sip_state_change(SIP_STATE_CALLING);
                break;
                
            case SIP_EVENT_CALL_CONNECTED:
                ESP_LOGI(TAG, "SIP call connected");
                app_controller_handle_sip_state_change(SIP_STATE_CONNECTED);
                break;
                
            case SIP_EVENT_CALL_ENDED:
                ESP_LOGI(TAG, "SIP call ended");
                app_controller_handle_sip_state_change(SIP_STATE_IDLE);
                break;
                
            case SIP_EVENT_CALL_FAILED:
                ESP_LOGE(TAG, "SIP call failed: %s", data->data.error.error_message);
                app_controller_handle_sip_state_change(SIP_STATE_IDLE);
                app_controller_handle_error(data->data.error.error_code, data->data.error.error_message);
                break;
                
            case SIP_EVENT_DTMF_RECEIVED:
                ESP_LOGI(TAG, "DTMF received: %c", data->data.dtmf.digit);
                app_controller_handle_dtmf(data->data.dtmf.digit);
                break;
                
            default:
                ESP_LOGW(TAG, "Unknown SIP event: %d", data->event_type);
                break;
        }
    }
}

// DTMF callback function
static void dtmf_callback(char digit, void *user_data)
{
    ESP_LOGI(TAG, "DTMF callback: digit %c", digit);
    
    // Forward DTMF to application controller
    esp_err_t result = app_controller_handle_dtmf(digit);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle DTMF digit: %s", esp_err_to_name(result));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 SIP Door Station starting...");
    
    // Initialize default event loop
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return;
    }
    
    // Initialize configuration manager (merges build-time and runtime config)
    esp_err_t config_result = config_manager_init();
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize configuration manager");
        return;
    }
    
    // Load and display current configuration
    door_station_config_t config;
    config_result = config_manager_load(&config);
    if (config_result == ESP_OK) {
        ESP_LOGI(TAG, "Configuration loaded successfully:");
        ESP_LOGI(TAG, "  Wi-Fi SSID: %s", strlen(config.wifi_ssid) > 0 ? config.wifi_ssid : "[not configured]");
        ESP_LOGI(TAG, "  SIP User: %s", strlen(config.sip_user) > 0 ? config.sip_user : "[not configured]");
        ESP_LOGI(TAG, "  SIP Domain: %s", strlen(config.sip_domain) > 0 ? config.sip_domain : "[not configured]");
        ESP_LOGI(TAG, "  SIP Callee: %s", strlen(config.sip_callee) > 0 ? config.sip_callee : "[not configured]");
        ESP_LOGI(TAG, "  Web Port: %" PRIu16, config.web_port);
        ESP_LOGI(TAG, "  Door Pulse Duration: %" PRIu32 " ms", config.door_pulse_duration);
        
        // Validate configuration
        config_validation_error_t validation = config_manager_validate(&config);
        if (validation == CONFIG_VALIDATION_OK) {
            ESP_LOGI(TAG, "✓ Configuration is valid");
        } else {
            ESP_LOGW(TAG, "⚠ Configuration validation warning: %s", 
                     config_manager_get_validation_error_message(validation));
        }
    } else {
        ESP_LOGE(TAG, "Failed to load configuration");
    }
    
    // Initialize I/O manager
    ret = io_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I/O manager");
        return;
    }
    
    // Register event handlers
    ret = esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_PRESSED, 
                                     button_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register button pressed event handler");
        return;
    }
    
    ret = esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_RELEASED, 
                                     button_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register button released event handler");
        return;
    }
    
    ret = esp_event_handler_register(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                                     relay_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register relay state change event handler");
        return;
    }
    
    // Initialize application controller
    ret = app_controller_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize application controller");
        return;
    }
    
    // Initialize SIP manager if configuration is valid
    if (config_result == ESP_OK && config_manager_validate(&config) == CONFIG_VALIDATION_OK) {
        if (strlen(config.sip_user) > 0 && strlen(config.sip_domain) > 0) {
            sip_config_t sip_config = {
                .port = 5060,
                .registration_timeout = 60,
                .call_timeout = 30
            };
            
            strncpy(sip_config.user, config.sip_user, sizeof(sip_config.user) - 1);
            strncpy(sip_config.domain, config.sip_domain, sizeof(sip_config.domain) - 1);
            strncpy(sip_config.password, config.sip_password, sizeof(sip_config.password) - 1);
            strncpy(sip_config.callee, config.sip_callee, sizeof(sip_config.callee) - 1);
            
            ret = sip_manager_init(&sip_config);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "SIP manager initialized successfully");
                
                // Register DTMF callback
                ret = sip_manager_register_dtmf_callback(dtmf_callback, NULL);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to register DTMF callback: %s", esp_err_to_name(ret));
                }
                
                // Start SIP manager
                ret = sip_manager_start();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to start SIP manager: %s", esp_err_to_name(ret));
                } else {
                    ESP_LOGI(TAG, "SIP manager started successfully");
                }
            } else {
                ESP_LOGE(TAG, "Failed to initialize SIP manager: %s", esp_err_to_name(ret));
            }
        } else {
            ESP_LOGW(TAG, "SIP configuration incomplete - SIP manager not started");
        }
    } else {
        ESP_LOGW(TAG, "Configuration invalid - SIP manager not started");
    }
    
    ESP_LOGI(TAG, "System initialized successfully");
    ESP_LOGI(TAG, "Press the boot button to test I/O functionality");
    
    // TODO: Initialize WiFi and web server
    // When WiFi is connected and web server is started, the IP address will be logged:
    // - web_server_init(config.web_port) will log the initial URL
    // - web_server_log_url() can be called when WiFi reconnects to show the URL again
    
    // Start main application event loop (this function does not return)
    ESP_LOGI(TAG, "Starting main application event loop");
    app_controller_start_event_loop();
}