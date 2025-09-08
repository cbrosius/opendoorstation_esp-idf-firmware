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
                // TODO: Initiate SIP call when SIP manager is implemented
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
        // TODO: Update web interface when web server is implemented
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
    
    ESP_LOGI(TAG, "System initialized successfully");
    ESP_LOGI(TAG, "Press the boot button to test I/O functionality");
    
    // Main application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}