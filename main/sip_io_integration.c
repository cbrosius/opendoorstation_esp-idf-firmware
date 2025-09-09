#include "sip_io_integration.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "sip_io_integration";

// Integration state
static struct {
    sip_io_config_t config;
    bool initialized;
    bool active;
    TimerHandle_t hangup_timer;
    bool door_opened_in_call;
} integration = {0};

// Forward declarations
static void sip_dtmf_command_handler(dtmf_command_t command, uint32_t param, void *user_data);
static void button_event_handler(bool pressed);
static void hangup_timer_callback(TimerHandle_t xTimer);
static esp_err_t execute_door_open_command(uint32_t pulse_duration);
static esp_err_t execute_status_request_command(void);
static esp_err_t execute_hangup_command(void);

/**
 * @brief Handle DTMF commands from SIP manager
 */
static void sip_dtmf_command_handler(dtmf_command_t command, uint32_t param, void *user_data) {
    if (!integration.active) {
        ESP_LOGW(TAG, "Integration not active, ignoring DTMF command %d", command);
        return;
    }
    
    ESP_LOGI(TAG, "Processing DTMF command: %d with param: %lu", command, param);
    
    switch (command) {
        case DTMF_CMD_DOOR_OPEN:
            {
                uint32_t pulse_duration = (param > 0) ? param : integration.config.door_pulse_duration_ms;
                esp_err_t ret = execute_door_open_command(pulse_duration);
                if (ret == ESP_OK) {
                    integration.door_opened_in_call = true;
                    
                    // Schedule auto hangup if enabled
                    if (integration.config.auto_hangup_after_door_open && integration.hangup_timer != NULL) {
                        xTimerChangePeriod(integration.hangup_timer, 
                                         pdMS_TO_TICKS(integration.config.hangup_delay_ms), 0);
                        xTimerStart(integration.hangup_timer, 0);
                        ESP_LOGI(TAG, "Scheduled auto hangup in %lu ms", integration.config.hangup_delay_ms);
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to open door: %s", esp_err_to_name(ret));
                }
            }
            break;
            
        case DTMF_CMD_STATUS_REQUEST:
            execute_status_request_command();
            break;
            
        case DTMF_CMD_HANGUP:
            execute_hangup_command();
            break;
            
        case DTMF_CMD_DOOR_CLOSE:
            // Not implemented for this door station (no door close relay)
            ESP_LOGW(TAG, "Door close command not supported");
            break;
            
        case DTMF_CMD_CUSTOM:
            ESP_LOGI(TAG, "Custom DTMF command received with param %lu", param);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown DTMF command: %d", command);
            break;
    }
}

/**
 * @brief Handle physical button events
 */
static void button_event_handler(bool pressed) {
    if (!integration.active) {
        return;
    }
    
    if (pressed) {
        ESP_LOGI(TAG, "Physical button pressed - opening door");
        
        // Open door with configured duration
        esp_err_t ret = execute_door_open_command(integration.config.door_pulse_duration_ms);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open door from button press: %s", esp_err_to_name(ret));
        }
    }
}

/**
 * @brief Timer callback for auto hangup
 */
static void hangup_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Auto hangup timer expired");
    execute_hangup_command();
}

/**
 * @brief Execute door open command
 */
static esp_err_t execute_door_open_command(uint32_t pulse_duration) {
    ESP_LOGI(TAG, "Opening door with %lu ms pulse", pulse_duration);
    
    esp_err_t ret = io_manager_pulse_relay(RELAY_DOOR, pulse_duration);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Door relay pulsed successfully");
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Door relay protection active - pulse rejected");
    } else {
        ESP_LOGE(TAG, "Failed to pulse door relay: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief Execute status request command
 */
static esp_err_t execute_status_request_command(void) {
    ESP_LOGI(TAG, "Status request received");
    
    if (integration.config.status_feedback_enabled) {
        // Get current relay states
        relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
        relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
        
        // Get SIP call statistics
        sip_call_stats_t call_stats;
        esp_err_t ret = sip_manager_get_call_stats(&call_stats);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "System Status - Door: %s, Light: %s, Calls: %lu/%lu", 
                     door_state == RELAY_STATE_ON ? "ON" : "OFF",
                     light_state == RELAY_STATE_ON ? "ON" : "OFF",
                     call_stats.successful_calls,
                     call_stats.total_calls_made);
        }
        
        // In a real implementation, this could send status via SIP INFO or other mechanism
        // For now, we just log the status
    }
    
    return ESP_OK;
}

/**
 * @brief Execute hangup command
 */
static esp_err_t execute_hangup_command(void) {
    ESP_LOGI(TAG, "Hanging up call");
    
    // Stop auto hangup timer if running
    if (integration.hangup_timer != NULL) {
        xTimerStop(integration.hangup_timer, 0);
    }
    
    // End the SIP call
    esp_err_t ret = sip_manager_end_call();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Call ended successfully");
        integration.door_opened_in_call = false;
    } else {
        ESP_LOGE(TAG, "Failed to end call: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t sip_io_integration_init(const sip_io_config_t *config) {
    if (integration.initialized) {
        ESP_LOGW(TAG, "SIP-IO integration already initialized");
        return ESP_OK;
    }
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Initializing SIP-IO integration");
    
    // Copy configuration
    memcpy(&integration.config, config, sizeof(sip_io_config_t));
    
    // Validate configuration
    if (integration.config.door_pulse_duration_ms < 100 || integration.config.door_pulse_duration_ms > 10000) {
        ESP_LOGE(TAG, "Invalid door pulse duration: %lu ms", integration.config.door_pulse_duration_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (integration.config.hangup_delay_ms == 0 || integration.config.hangup_delay_ms > 60000) {
        ESP_LOGE(TAG, "Invalid hangup delay: %lu ms (must be between 1 and 60000)", integration.config.hangup_delay_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Create hangup timer
    integration.hangup_timer = xTimerCreate(
        "sip_hangup_timer",
        pdMS_TO_TICKS(integration.config.hangup_delay_ms),
        pdFALSE,  // One-shot timer
        NULL,
        hangup_timer_callback
    );
    
    if (integration.hangup_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create hangup timer");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize state
    integration.active = false;
    integration.door_opened_in_call = false;
    integration.initialized = true;
    
    ESP_LOGI(TAG, "SIP-IO integration initialized successfully");
    ESP_LOGI(TAG, "Door pulse duration: %lu ms", integration.config.door_pulse_duration_ms);
    ESP_LOGI(TAG, "Auto hangup: %s", integration.config.auto_hangup_after_door_open ? "enabled" : "disabled");
    if (integration.config.auto_hangup_after_door_open) {
        ESP_LOGI(TAG, "Hangup delay: %lu ms", integration.config.hangup_delay_ms);
    }
    
    return ESP_OK;
}

esp_err_t sip_io_integration_start(void) {
    if (!integration.initialized) {
        ESP_LOGE(TAG, "Integration not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (integration.active) {
        ESP_LOGW(TAG, "Integration already active");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting SIP-IO integration");
    
    // Register DTMF command callback with SIP manager
    esp_err_t ret = sip_manager_register_dtmf_command_callback(sip_dtmf_command_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register DTMF command callback: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register button callback with I/O manager
    ret = io_manager_register_button_callback(button_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register button callback: %s", esp_err_to_name(ret));
        return ret;
    }
    
    integration.active = true;
    integration.door_opened_in_call = false;
    
    ESP_LOGI(TAG, "SIP-IO integration started successfully");
    return ESP_OK;
}

esp_err_t sip_io_integration_stop(void) {
    if (!integration.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping SIP-IO integration");
    
    // Stop hangup timer
    if (integration.hangup_timer != NULL) {
        xTimerStop(integration.hangup_timer, 0);
    }
    
    integration.active = false;
    integration.door_opened_in_call = false;
    
    ESP_LOGI(TAG, "SIP-IO integration stopped");
    return ESP_OK;
}

esp_err_t sip_io_integration_update_config(const sip_io_config_t *config) {
    if (!integration.initialized) {
        ESP_LOGE(TAG, "Integration not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Updating SIP-IO integration configuration");
    
    // Validate new configuration
    if (config->door_pulse_duration_ms < 100 || config->door_pulse_duration_ms > 10000) {
        ESP_LOGE(TAG, "Invalid door pulse duration: %lu ms", config->door_pulse_duration_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->hangup_delay_ms > 60000) {
        ESP_LOGE(TAG, "Invalid hangup delay: %lu ms", config->hangup_delay_ms);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Update configuration
    memcpy(&integration.config, config, sizeof(sip_io_config_t));
    
    ESP_LOGI(TAG, "SIP-IO integration configuration updated");
    return ESP_OK;
}

esp_err_t sip_io_integration_get_config(sip_io_config_t *config) {
    if (!integration.initialized) {
        ESP_LOGE(TAG, "Integration not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(config, &integration.config, sizeof(sip_io_config_t));
    return ESP_OK;
}

bool sip_io_integration_is_active(void) {
    return integration.active;
}