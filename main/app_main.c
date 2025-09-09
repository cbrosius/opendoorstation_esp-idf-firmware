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
#include "wifi_manager.h"
#include "web_server.h"

static const char *TAG = "sip_door_station";

// Test mode support
#ifdef CONFIG_RUN_TESTS
#include "unity.h"
// Weak function declaration - will be overridden by test component if linked
void __attribute__((weak)) app_main_tests(void) {
    ESP_LOGE(TAG, "Test function not linked - test component not included in build");
}
#endif

// Event handlers
static void button_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS) {
        switch (event_id) {
            case IO_EVENT_BUTTON_PRESSED: {
                io_button_event_data_t* data = (io_button_event_data_t*)event_data;
                ESP_LOGI(TAG, "Button pressed at timestamp: %" PRIu32, data->timestamp);
                
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
        
        esp_err_t result = app_controller_handle_relay_state_change(data->relay, data->new_state);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to handle relay state change: %s", esp_err_to_name(result));
        }
    }
}

// WiFi event callback function
static void wifi_event_callback(wifi_state_t state, const wifi_info_t *info, void *user_data)
{
    ESP_LOGI(TAG, "WiFi state changed to: %s", wifi_manager_get_state_string(state));
    
    switch (state) {
        case WIFI_STATE_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected successfully!");
            ESP_LOGI(TAG, "SSID: %s", info->ssid);
            ESP_LOGI(TAG, "IP Address: %s", info->ip_address);
            ESP_LOGI(TAG, "Signal Strength: %d dBm", info->rssi);
            
            app_controller_start_services();
            break;
            
        case WIFI_STATE_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi disconnected");
            web_server_stop();
            sip_manager_stop();
            break;
            
        case WIFI_STATE_CONNECTING:
            ESP_LOGI(TAG, "Connecting to WiFi...");
            break;
            
        case WIFI_STATE_ERROR:
            ESP_LOGE(TAG, "WiFi connection failed after retries");
            break;
            
        default:
            break;
    }
}

void app_main(void)
{
#ifdef CONFIG_RUN_TESTS
    ESP_LOGI(TAG, "ESP32 SIP Door Station - Unity Test Framework");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Starting comprehensive test suite...");
    ESP_LOGI(TAG, "");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    app_main_tests();
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Test execution completed.");
    ESP_LOGI(TAG, "Check the output above for test results.");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "System running... Press reset to run tests again.");
    }
#else
    ESP_LOGI(TAG, "ESP32 SIP Door Station starting...");
    
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return;
    }
    
    esp_err_t config_result = config_manager_init();
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize configuration manager");
        return;
    }
    
    door_station_config_t config;
    config_result = config_manager_get_current(&config);
    if (config_result == ESP_OK) {
        ESP_LOGI(TAG, "Configuration loaded successfully:");
        ESP_LOGI(TAG, "  Wi-Fi SSID: %s", strlen(config.wifi_ssid) > 0 ? config.wifi_ssid : "[not configured]");
        ESP_LOGI(TAG, "  SIP User: %s", strlen(config.sip_user) > 0 ? config.sip_user : "[not configured]");
        ESP_LOGI(TAG, "  SIP Domain: %s", strlen(config.sip_domain) > 0 ? config.sip_domain : "[not configured]");
        ESP_LOGI(TAG, "  SIP Callee: %s", strlen(config.sip_callee) > 0 ? config.sip_callee : "[not configured]");
        ESP_LOGI(TAG, "  Web Port: %" PRIu16, config.web_port);
        ESP_LOGI(TAG, "  Door Pulse Duration: %" PRIu32 " ms", config.door_pulse_duration);
        
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
    
    ret = io_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I/O manager");
        return;
    }
    
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
    
    ret = app_controller_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize application controller");
        return;
    }
    
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi manager: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = wifi_manager_register_callback(wifi_event_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register WiFi callback: %s", esp_err_to_name(ret));
    }
    
    if (config_result == ESP_OK && strlen(config.wifi_ssid) > 0) {
        ESP_LOGI(TAG, "Connecting to WiFi network: %s", config.wifi_ssid);
        ret = wifi_manager_connect(config.wifi_ssid, config.wifi_password);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start WiFi connection: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "WiFi not configured - web interface will not be available");
        ESP_LOGI(TAG, "Configure WiFi settings and restart to enable web interface");
    }
    
    ESP_LOGI(TAG, "System initialized successfully");
    ESP_LOGI(TAG, "Press the boot button to test I/O functionality");
    ESP_LOGI(TAG, "Web interface will be available once WiFi connects");
    
    app_controller_start_event_loop();
#endif
}