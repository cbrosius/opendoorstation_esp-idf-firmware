#include "sip_manager.h"
#include "esp_sip.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>
#include <time.h>

static const char *TAG = "sip_manager";

// SIP manager state
static struct {
    sip_config_t config;
    sip_state_t state;
    dtmf_callback_t dtmf_callback;
    void *dtmf_user_data;
    bool initialized;
    bool call_active;
    uint32_t call_start_time;
    uint32_t last_dtmf_time;
    esp_sip_client_handle_t sip_client;
    TimerHandle_t call_timeout_timer;
    
    // Call statistics
    sip_call_stats_t call_stats;
    
    // DTMF command processing
    dtmf_command_mapping_t dtmf_mappings[12]; // Max 12 DTMF digits
    size_t dtmf_mapping_count;
    dtmf_command_callback_t dtmf_command_callback;
    void *dtmf_command_user_data;
    bool dtmf_processing_enabled;
} sip_manager = {0};

// Event declarations
ESP_EVENT_DECLARE_BASE(SIP_EVENTS);
ESP_EVENT_DEFINE_BASE(SIP_EVENTS);

// Forward declarations
static void sip_event_callback(esp_sip_event_data_t *event_data, void *user_data);
static void call_timeout_callback(TimerHandle_t xTimer);
static esp_err_t sip_manager_set_state(sip_state_t new_state);
static esp_err_t sip_manager_post_event(sip_event_type_t event_type, const void *event_data);
static void process_dtmf_digit(char digit);
static dtmf_command_t map_dtmf_to_command(char digit, uint32_t *param);

/**
 * @brief Validate SIP configuration
 */
static esp_err_t validate_sip_config(const sip_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(config->user) < 3 || strlen(config->user) > 31) {
        ESP_LOGE(TAG, "Invalid SIP user length");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(config->domain) == 0 || strlen(config->domain) > 63) {
        ESP_LOGE(TAG, "Invalid SIP domain length");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(config->password) == 0 || strlen(config->password) > 63) {
        ESP_LOGE(TAG, "Invalid SIP password length");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config->port == 0) {
        ESP_LOGE(TAG, "Invalid SIP port");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->call_timeout == 0) {
        ESP_LOGE(TAG, "Invalid call timeout");
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

/**
 * @brief Set SIP manager state and post event if changed
 */
static esp_err_t sip_manager_set_state(sip_state_t new_state) {
    if (sip_manager.state != new_state) {
        sip_state_t old_state = sip_manager.state;
        sip_manager.state = new_state;
        
        ESP_LOGI(TAG, "State changed: %d -> %d", old_state, new_state);
        
        // Post appropriate events based on state change
        switch (new_state) {
            case SIP_STATE_REGISTERED:
                sip_manager_post_event(SIP_EVENT_REGISTERED, NULL);
                break;
            case SIP_STATE_CALLING:
                sip_manager_post_event(SIP_EVENT_CALL_STARTED, NULL);
                break;
            case SIP_STATE_CONNECTED:
                sip_manager_post_event(SIP_EVENT_CALL_CONNECTED, NULL);
                break;
            case SIP_STATE_IDLE:
                if (old_state == SIP_STATE_CONNECTED || old_state == SIP_STATE_CALLING) {
                    sip_manager_post_event(SIP_EVENT_CALL_ENDED, NULL);
                }
                break;
            case SIP_STATE_ERROR:
                if (old_state == SIP_STATE_REGISTERING) {
                    sip_manager_post_event(SIP_EVENT_REGISTRATION_FAILED, NULL);
                } else if (old_state == SIP_STATE_CALLING) {
                    sip_manager_post_event(SIP_EVENT_CALL_FAILED, NULL);
                }
                break;
            default:
                break;
        }
    }
    return ESP_OK;
}

/**
 * @brief Post SIP event to event loop
 */
static esp_err_t sip_manager_post_event(sip_event_type_t event_type, const void *event_data) {
    sip_event_data_t sip_event = {
        .event_type = event_type
    };
    
    if (event_data != NULL) {
        memcpy(&sip_event.data, event_data, sizeof(sip_event.data));
    }
    
    return esp_event_post(SIP_EVENTS, event_type, &sip_event, sizeof(sip_event), portMAX_DELAY);
}

/**
 * @brief Map DTMF digit to command
 */
static dtmf_command_t map_dtmf_to_command(char digit, uint32_t *param) {
    if (!sip_manager.dtmf_processing_enabled) {
        return DTMF_CMD_NONE;
    }
    
    for (size_t i = 0; i < sip_manager.dtmf_mapping_count; i++) {
        if (sip_manager.dtmf_mappings[i].digit == digit && sip_manager.dtmf_mappings[i].enabled) {
            if (param != NULL) {
                *param = sip_manager.dtmf_mappings[i].param;
            }
            return sip_manager.dtmf_mappings[i].command;
        }
    }
    
    return DTMF_CMD_NONE;
}

/**
 * @brief Process received DTMF digit
 */
static void process_dtmf_digit(char digit) {
    ESP_LOGI(TAG, "Processing DTMF digit: %c", digit);
    
    // Map digit to command
    uint32_t param = 0;
    dtmf_command_t command = map_dtmf_to_command(digit, &param);
    
    if (command != DTMF_CMD_NONE) {
        ESP_LOGI(TAG, "DTMF digit %c mapped to command %d with param %lu", digit, command, param);
        
        // Call command callback if registered
        if (sip_manager.dtmf_command_callback != NULL) {
            sip_manager.dtmf_command_callback(command, param, sip_manager.dtmf_command_user_data);
        }
    } else {
        ESP_LOGD(TAG, "DTMF digit %c not mapped to any command", digit);
    }
}

/**
 * @brief SIP event callback - handles events from esp_sip library
 */
static void sip_event_callback(esp_sip_event_data_t *event_data, void *user_data) {
    if (!event_data) {
        return;
    }
    
    ESP_LOGI(TAG, "SIP event received: %d", event_data->event);
    
    switch (event_data->event) {
        case ESP_SIP_EVENT_REGISTERED:
            sip_manager_set_state(SIP_STATE_REGISTERED);
            break;
            
        case ESP_SIP_EVENT_REGISTRATION_FAILED:
            sip_manager_set_state(SIP_STATE_ERROR);
            break;
            
        case ESP_SIP_EVENT_CALL_STARTED:
            sip_manager.call_stats.total_calls_made++;
            sip_manager_set_state(SIP_STATE_CALLING);
            break;
            
        case ESP_SIP_EVENT_CALL_CONNECTED:
            sip_manager.call_start_time = esp_timer_get_time() / 1000000;
            sip_manager.call_active = true;
            sip_manager_set_state(SIP_STATE_CONNECTED);
            break;
            
        case ESP_SIP_EVENT_CALL_ENDED:
            // Update call statistics
            if (sip_manager.call_active && sip_manager.call_start_time > 0) {
                uint32_t call_duration = sip_manager_get_call_duration();
                sip_manager.call_stats.total_call_duration += call_duration;
                sip_manager.call_stats.successful_calls++;
                sip_manager.call_stats.last_call_end_reason = 0; // Normal end
                ESP_LOGI(TAG, "Call ended normally, duration: %lu seconds", call_duration);
            }
            
            sip_manager.call_active = false;
            sip_manager.call_start_time = 0;
            sip_manager_set_state(SIP_STATE_REGISTERED);
            break;
            
        case ESP_SIP_EVENT_CALL_FAILED:
            // Update failure statistics
            sip_manager.call_stats.failed_calls++;
            sip_manager.call_stats.last_call_end_reason = event_data->data.error.code;
            ESP_LOGW(TAG, "Call failed: %s", event_data->data.error.message);
            
            sip_manager.call_active = false;
            sip_manager.call_start_time = 0;
            sip_manager_set_state(SIP_STATE_ERROR);
            break;
            
        case ESP_SIP_EVENT_DTMF_RECEIVED:
            sip_manager.last_dtmf_time = esp_timer_get_time() / 1000000;
            
            // Process DTMF digit for command mapping
            process_dtmf_digit(event_data->data.dtmf.digit);
            
            // Call registered raw DTMF callback
            if (sip_manager.dtmf_callback != NULL) {
                sip_manager.dtmf_callback(event_data->data.dtmf.digit, sip_manager.dtmf_user_data);
            }
            
            // Post DTMF event
            sip_event_data_t sip_event = {
                .event_type = SIP_EVENT_DTMF_RECEIVED,
                .data.dtmf = {
                    .digit = event_data->data.dtmf.digit,
                    .timestamp = sip_manager.last_dtmf_time
                }
            };
            sip_manager_post_event(SIP_EVENT_DTMF_RECEIVED, &sip_event.data);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown SIP event: %d", event_data->event);
            break;
    }
}

/**
 * @brief Call timeout callback - ends call if timeout reached
 */
static void call_timeout_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Call timeout reached");
    
    if (sip_manager.state == SIP_STATE_CALLING || sip_manager.state == SIP_STATE_CONNECTED) {
        ESP_LOGW(TAG, "Ending call due to timeout");
        
        // Post timeout event before ending call
        sip_event_data_t timeout_event = {
            .event_type = SIP_EVENT_CALL_FAILED,
            .data.error = {
                .error_code = -1,
                .error_message = "Call timeout"
            }
        };
        sip_manager_post_event(SIP_EVENT_CALL_FAILED, &timeout_event.data);
        
        sip_manager_end_call();
    }
}



esp_err_t sip_manager_init(const sip_config_t *config) {
    if (sip_manager.initialized) {
        ESP_LOGW(TAG, "SIP manager already initialized");
        return ESP_OK;
    }
    
    esp_err_t ret = validate_sip_config(config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Initializing SIP manager");
    
    // Copy configuration
    memcpy(&sip_manager.config, config, sizeof(sip_config_t));
    
    // Initialize state
    sip_manager.state = SIP_STATE_IDLE;
    sip_manager.dtmf_callback = NULL;
    sip_manager.dtmf_user_data = NULL;
    sip_manager.call_active = false;
    sip_manager.call_start_time = 0;
    sip_manager.last_dtmf_time = 0;
    
    // Initialize DTMF command processing with default mappings
    sip_manager.dtmf_processing_enabled = true;
    sip_manager.dtmf_command_callback = NULL;
    sip_manager.dtmf_command_user_data = NULL;
    sip_manager.dtmf_mapping_count = 0;
    
    // Set up default DTMF command mappings
    dtmf_command_mapping_t default_mappings[] = {
        {'1', DTMF_CMD_DOOR_OPEN, 0, true},      // '1' opens door
        {'0', DTMF_CMD_HANGUP, 0, true},         // '0' hangs up
        {'*', DTMF_CMD_STATUS_REQUEST, 0, true}, // '*' requests status
    };
    
    size_t default_count = sizeof(default_mappings) / sizeof(default_mappings[0]);
    if (default_count <= sizeof(sip_manager.dtmf_mappings) / sizeof(sip_manager.dtmf_mappings[0])) {
        memcpy(sip_manager.dtmf_mappings, default_mappings, sizeof(default_mappings));
        sip_manager.dtmf_mapping_count = default_count;
        ESP_LOGI(TAG, "Initialized %zu default DTMF command mappings", default_count);
    }
    
    // Create call timeout timer
    sip_manager.call_timeout_timer = xTimerCreate(
        "sip_call_timer",
        pdMS_TO_TICKS(sip_manager.config.call_timeout * 1000),
        pdFALSE,  // One-shot timer
        NULL,
        call_timeout_callback
    );
    
    if (sip_manager.call_timeout_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create call timeout timer");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize esp_sip client
    esp_sip_config_t esp_sip_config = {
        .username = sip_manager.config.user,
        .password = sip_manager.config.password,
        .server_uri = sip_manager.config.domain,
        .port = sip_manager.config.port,
        .registration_timeout_sec = sip_manager.config.registration_timeout,
        .call_timeout_sec = sip_manager.config.call_timeout
    };
    
    esp_err_t sip_ret = esp_sip_init(&esp_sip_config, sip_event_callback, NULL, &sip_manager.sip_client);
    if (sip_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize esp_sip client");
        xTimerDelete(sip_manager.call_timeout_timer, 0);
        return sip_ret;
    }
    
    sip_manager.initialized = true;
    
    ESP_LOGI(TAG, "SIP manager initialized successfully");
    ESP_LOGI(TAG, "SIP User: %s", sip_manager.config.user);
    ESP_LOGI(TAG, "SIP Domain: %s", sip_manager.config.domain);
    ESP_LOGI(TAG, "SIP Port: %d", sip_manager.config.port);
    
    return ESP_OK;
}

esp_err_t sip_manager_start(void) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sip_manager.state != SIP_STATE_IDLE) {
        ESP_LOGW(TAG, "SIP manager already started");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting SIP manager");
    
    // Start registration process
    sip_manager_set_state(SIP_STATE_REGISTERING);
    
    // Start esp_sip client
    esp_err_t ret = esp_sip_start(sip_manager.sip_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start esp_sip client");
        return ret;
    }
    
    ESP_LOGI(TAG, "SIP manager started, attempting registration");
    return ESP_OK;
}

esp_err_t sip_manager_stop(void) {
    if (!sip_manager.initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping SIP manager");
    
    // End any active call
    if (sip_manager.call_active) {
        sip_manager_end_call();
    }
    
    // Stop call timeout timer
    xTimerStop(sip_manager.call_timeout_timer, 0);
    
    // Stop esp_sip client
    if (sip_manager.sip_client != NULL) {
        esp_sip_stop(sip_manager.sip_client);
    }
    
    // Set state to idle
    sip_manager_set_state(SIP_STATE_IDLE);
    
    // Mark as not initialized
    sip_manager.initialized = false;
    
    ESP_LOGI(TAG, "SIP manager stopped");
    return ESP_OK;
}

esp_err_t sip_manager_start_call(const char *uri) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sip_manager.state != SIP_STATE_REGISTERED) {
        ESP_LOGE(TAG, "SIP not registered, cannot start call");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (sip_manager.call_active) {
        ESP_LOGW(TAG, "Call already active");
        return ESP_ERR_INVALID_STATE;
    }
    
    const char *target_uri = uri ? uri : sip_manager.config.callee;
    if (strlen(target_uri) == 0) {
        ESP_LOGE(TAG, "No target URI specified for call");
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Starting call to: %s", target_uri);
    
    // Set state to calling
    sip_manager_set_state(SIP_STATE_CALLING);
    
    // Start call timeout timer
    if (xTimerStart(sip_manager.call_timeout_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start call timeout timer");
        sip_manager_set_state(SIP_STATE_REGISTERED);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Initiate call via esp_sip library
    esp_err_t ret = esp_sip_call(sip_manager.sip_client, target_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initiate call");
        xTimerStop(sip_manager.call_timeout_timer, 0);
        sip_manager_set_state(SIP_STATE_REGISTERED);
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t sip_manager_end_call(void) {
    if (!sip_manager.call_active && 
        sip_manager.state != SIP_STATE_CALLING && 
        sip_manager.state != SIP_STATE_CONNECTED) {
        ESP_LOGW(TAG, "No active call to end");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Ending call");
    
    // Stop call timeout timer
    xTimerStop(sip_manager.call_timeout_timer, 0);
    
    // Reset call state
    sip_manager.call_active = false;
    sip_manager.call_start_time = 0;
    
    // Terminate call via esp_sip library
    esp_err_t ret = esp_sip_hangup(sip_manager.sip_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to hangup call");
    }
    
    // Set state back to registered (will be updated by callback)
    sip_manager_set_state(SIP_STATE_REGISTERED);
    
    return ESP_OK;
}

sip_state_t sip_manager_get_state(void) {
    return sip_manager.state;
}

esp_err_t sip_manager_register_dtmf_callback(dtmf_callback_t callback, void *user_data) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    sip_manager.dtmf_callback = callback;
    sip_manager.dtmf_user_data = user_data;
    
    ESP_LOGI(TAG, "DTMF callback registered");
    return ESP_OK;
}

bool sip_manager_is_call_active(void) {
    return sip_manager.call_active;
}

uint32_t sip_manager_get_call_duration(void) {
    if (!sip_manager.call_active || sip_manager.call_start_time == 0) {
        return 0;
    }
    
    uint32_t current_time = esp_timer_get_time() / 1000000;
    return current_time - sip_manager.call_start_time;
}

esp_err_t sip_manager_update_config(const sip_config_t *config) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = validate_sip_config(config);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ESP_LOGI(TAG, "Updating SIP configuration");
    
    // Stop if currently running
    bool was_running = (sip_manager.state != SIP_STATE_IDLE);
    if (was_running) {
        sip_manager_stop();
    }
    
    // Destroy old esp_sip client
    if (sip_manager.sip_client != NULL) {
        esp_sip_destroy(sip_manager.sip_client);
        sip_manager.sip_client = NULL;
    }
    
    // Update configuration
    memcpy(&sip_manager.config, config, sizeof(sip_config_t));
    
    // Reinitialize with new config
    esp_sip_config_t esp_sip_config = {
        .username = sip_manager.config.user,
        .password = sip_manager.config.password,
        .server_uri = sip_manager.config.domain,
        .port = sip_manager.config.port,
        .registration_timeout_sec = sip_manager.config.registration_timeout,
        .call_timeout_sec = sip_manager.config.call_timeout
    };
    
    ret = esp_sip_init(&esp_sip_config, sip_event_callback, NULL, &sip_manager.sip_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reinitialize esp_sip client");
        return ret;
    }
    
    // Restart if it was running
    if (was_running) {
        ret = sip_manager_start();
    }
    
    return ret;
}

esp_err_t sip_manager_get_call_stats(sip_call_stats_t *stats) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (stats == NULL) {
        ESP_LOGE(TAG, "Stats pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy current statistics
    memcpy(stats, &sip_manager.call_stats, sizeof(sip_call_stats_t));
    
    // Update current call duration if call is active
    if (sip_manager.call_active) {
        stats->current_call_duration = sip_manager_get_call_duration();
    } else {
        stats->current_call_duration = 0;
    }
    
    return ESP_OK;
}

esp_err_t sip_manager_reset_call_stats(void) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Resetting call statistics");
    
    // Reset all statistics except current call duration
    sip_manager.call_stats.total_calls_made = 0;
    sip_manager.call_stats.successful_calls = 0;
    sip_manager.call_stats.failed_calls = 0;
    sip_manager.call_stats.total_call_duration = 0;
    sip_manager.call_stats.last_call_end_reason = 0;
    
    return ESP_OK;
}

esp_err_t sip_manager_configure_dtmf_commands(const dtmf_command_mapping_t *mappings, size_t count) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (mappings == NULL && count > 0) {
        ESP_LOGE(TAG, "Invalid mappings pointer");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (count > sizeof(sip_manager.dtmf_mappings) / sizeof(sip_manager.dtmf_mappings[0])) {
        ESP_LOGE(TAG, "Too many DTMF mappings (max %zu)", 
                 sizeof(sip_manager.dtmf_mappings) / sizeof(sip_manager.dtmf_mappings[0]));
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Configuring %zu DTMF command mappings", count);
    
    // Validate all mappings first
    for (size_t i = 0; i < count; i++) {
        char digit = mappings[i].digit;
        if ((digit < '0' || digit > '9') && digit != '*' && digit != '#') {
            ESP_LOGE(TAG, "Invalid DTMF digit: %c", digit);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Copy mappings
    if (count > 0) {
        memcpy(sip_manager.dtmf_mappings, mappings, count * sizeof(dtmf_command_mapping_t));
    }
    sip_manager.dtmf_mapping_count = count;
    
    ESP_LOGI(TAG, "DTMF command mappings configured successfully");
    return ESP_OK;
}

esp_err_t sip_manager_register_dtmf_command_callback(dtmf_command_callback_t callback, void *user_data) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    sip_manager.dtmf_command_callback = callback;
    sip_manager.dtmf_command_user_data = user_data;
    
    ESP_LOGI(TAG, "DTMF command callback registered");
    return ESP_OK;
}

esp_err_t sip_manager_get_dtmf_commands(dtmf_command_mapping_t *mappings, size_t max_count, size_t *actual_count) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (actual_count == NULL) {
        ESP_LOGE(TAG, "actual_count pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    *actual_count = sip_manager.dtmf_mapping_count;
    
    if (mappings != NULL && max_count > 0) {
        size_t copy_count = (max_count < sip_manager.dtmf_mapping_count) ? max_count : sip_manager.dtmf_mapping_count;
        memcpy(mappings, sip_manager.dtmf_mappings, copy_count * sizeof(dtmf_command_mapping_t));
    }
    
    return ESP_OK;
}

esp_err_t sip_manager_set_dtmf_processing_enabled(bool enabled) {
    if (!sip_manager.initialized) {
        ESP_LOGE(TAG, "SIP manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    sip_manager.dtmf_processing_enabled = enabled;
    ESP_LOGI(TAG, "DTMF processing %s", enabled ? "enabled" : "disabled");
    
    return ESP_OK;
}