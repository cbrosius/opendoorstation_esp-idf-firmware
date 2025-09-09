#include "app_controller.h"
#include "error_handler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include "web_server.h"
#include "sip_manager.h"

static const char *TAG = "app_controller";

// Global system state
static system_state_t g_system_state = {0};
static SemaphoreHandle_t g_state_mutex = NULL;
static bool g_controller_initialized = false;
static uint32_t g_init_time = 0;

// Event loop handle
static esp_event_loop_handle_t g_app_event_loop = NULL;

// Forward declarations
static void app_controller_update_uptime(void);
static esp_err_t app_controller_transition_state(app_state_t new_state);
static void app_controller_log_state_transition(app_state_t old_state, app_state_t new_state);
static void services_init_task(void *arg);

esp_err_t app_controller_init(void)
{
    if (g_controller_initialized) {
        ESP_LOGW(TAG, "App controller already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing application controller");

    esp_err_t ret = error_handler_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize error handler: %s", esp_err_to_name(ret));
        return ret;
    }

    g_state_mutex = xSemaphoreCreateMutex();
    if (g_state_mutex == NULL) {
        ERROR_REPORT_SYSTEM(ERROR_SEVERITY_CRITICAL, "app_controller", ESP_ERR_NO_MEM, 
                           "Failed to create state mutex");
        return ESP_ERR_NO_MEM;
    }

    memset(&g_system_state, 0, sizeof(system_state_t));
    g_system_state.app_state = APP_STATE_INITIALIZING;
    g_system_state.sip_state = SIP_STATE_IDLE;
    g_system_state.door_relay_state = RELAY_STATE_OFF;
    g_system_state.light_relay_state = RELAY_STATE_OFF;
    g_system_state.button_pressed = false;
    g_system_state.sip_registered = false;
    g_init_time = esp_timer_get_time() / 1000000;

    esp_event_loop_args_t loop_args = {
        .queue_size = 10,
        .task_name = "app_event_loop",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = tskNO_AFFINITY
    };

    ret = esp_event_loop_create(&loop_args, &g_app_event_loop);
    if (ret != ESP_OK) {
        ERROR_REPORT_SYSTEM(ERROR_SEVERITY_CRITICAL, "app_controller", ret, 
                           "Failed to create application event loop");
        vSemaphoreDelete(g_state_mutex);
        return ret;
    }

    g_controller_initialized = true;
    ESP_LOGI(TAG, "Application controller initialized successfully");

    return ESP_OK;
}

esp_err_t app_controller_start_services(void)
{
    ESP_LOGI(TAG, "Starting network services in dedicated task...");
    BaseType_t task_ret = xTaskCreate(
        services_init_task,
        "services_init_task",
        8192, // Increased stack size for SPIFFS format and other initializations
        NULL,
        5,
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create services init task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void services_init_task(void *arg)
{
    ESP_LOGI(TAG, "Services init task started.");

    // Get configuration
    door_station_config_t config;
    esp_err_t config_result = config_manager_get_current(&config);
    if (config_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get configuration for services");
        vTaskDelete(NULL);
        return;
    }

    // Start web server
    esp_err_t result = web_server_init(config.web_port);
    if (result == ESP_OK) {
        ESP_LOGI(TAG, "Web server started successfully");
        web_server_log_url();
    } else {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(result));
    }

    // Start SIP manager
    if (config_manager_validate(&config) == CONFIG_VALIDATION_OK) {
        if (strlen(config.sip_user) > 0 && strlen(config.sip_domain) > 0) {
            sip_config_t sip_config = {
                .port = 5060,
            };
            strncpy(sip_config.user, config.sip_user, sizeof(sip_config.user) - 1);
            strncpy(sip_config.domain, config.sip_domain, sizeof(sip_config.domain) - 1);
            strncpy(sip_config.password, config.sip_password, sizeof(sip_config.password) - 1);
            strncpy(sip_config.callee, config.sip_callee, sizeof(sip_config.callee) - 1);

            result = sip_manager_init(&sip_config);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "SIP manager initialized successfully");
                result = sip_manager_start();
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to start SIP manager: %s", esp_err_to_name(result));
                }
            } else {
                ESP_LOGE(TAG, "Failed to initialize SIP manager: %s", esp_err_to_name(result));
            }
        } else {
            ESP_LOGW(TAG, "SIP configuration incomplete - SIP manager not started");
        }
    } else {
        ESP_LOGW(TAG, "Configuration invalid - SIP manager not started");
    }

    ESP_LOGI(TAG, "Services init task finished.");
    vTaskDelete(NULL);
}

esp_err_t app_controller_start_event_loop(void)
{
    if (!g_controller_initialized) {
        ESP_LOGE(TAG, "App controller not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting main application event loop");

    esp_err_t ret = app_controller_transition_state(APP_STATE_IDLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transition to idle state");
        return ret;
    }

    while (1) {
        app_controller_update_uptime();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return ESP_OK;
}

esp_err_t app_controller_get_system_state(system_state_t *state)
{
    if (state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        app_controller_update_uptime();
        memcpy(state, &g_system_state, sizeof(system_state_t));
        xSemaphoreGive(g_state_mutex);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t app_controller_set_app_state(app_state_t new_state)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return app_controller_transition_state(new_state);
}

esp_err_t app_controller_handle_button_press(void)
{
    if (!g_controller_initialized) {
        ERROR_REPORT_SYSTEM(ERROR_SEVERITY_WARNING, "app_controller", ESP_ERR_INVALID_STATE, 
                           "Button press handled before controller initialization");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Handling button press event");

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_state.button_pressed = true;

        switch (g_system_state.app_state) {
            case APP_STATE_IDLE:
                ESP_LOGI(TAG, "Button pressed in idle state - initiating call");
                app_controller_transition_state(APP_STATE_CALLING);
                g_system_state.call_start_time = esp_timer_get_time() / 1000000;
                ERROR_REPORT_SYSTEM(ERROR_SEVERITY_INFO, "app_controller", ESP_OK, 
                                   "Call initiated by button press");
                break;

            case APP_STATE_CALLING:
                ESP_LOGW(TAG, "Button pressed while calling - ignoring");
                ERROR_REPORT_SYSTEM(ERROR_SEVERITY_WARNING, "app_controller", ESP_OK, 
                                   "Button pressed during active call - ignored");
                break;

            case APP_STATE_CONNECTED:
                ESP_LOGI(TAG, "Button pressed during call - ending call");
                app_controller_transition_state(APP_STATE_IDLE);
                ERROR_REPORT_SYSTEM(ERROR_SEVERITY_INFO, "app_controller", ESP_OK, 
                                   "Call ended by button press");
                break;

            case APP_STATE_ERROR:
                ESP_LOGI(TAG, "Button pressed in error state - attempting recovery");
                app_controller_transition_state(APP_STATE_IDLE);
                ERROR_REPORT_SYSTEM(ERROR_SEVERITY_INFO, "app_controller", ESP_OK, 
                                   "Error recovery initiated by button press");
                break;

            default:
                ERROR_REPORT_SYSTEM(ERROR_SEVERITY_WARNING, "app_controller", ESP_OK, 
                                   "Button pressed in unknown state: %d", g_system_state.app_state);
                break;
        }

        xSemaphoreGive(g_state_mutex);
    } else {
        ERROR_REPORT_SYSTEM(ERROR_SEVERITY_ERROR, "app_controller", ESP_ERR_TIMEOUT, 
                           "Failed to acquire state mutex for button press handling");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_controller_handle_dtmf(char digit)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Handling DTMF digit: %c", digit);

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_state.last_dtmf_time = esp_timer_get_time() / 1000000;

        if (g_system_state.app_state != APP_STATE_CONNECTED) {
            ESP_LOGW(TAG, "DTMF received but call not connected (state: %s)", 
                     app_controller_get_state_string(g_system_state.app_state));
            xSemaphoreGive(g_state_mutex);
            return ESP_ERR_INVALID_STATE;
        }

        switch (digit) {
            case '1':
                ESP_LOGI(TAG, "DTMF '1' - pulsing door relay");
                io_manager_pulse_relay(RELAY_DOOR, 2000);
                break;

            case '2':
                ESP_LOGI(TAG, "DTMF '2' - toggling light relay");
                io_manager_toggle_relay(RELAY_LIGHT);
                break;

            default:
                ESP_LOGW(TAG, "Unknown DTMF digit: %c", digit);
                break;
        }

        xSemaphoreGive(g_state_mutex);
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_controller_handle_sip_state_change(sip_state_t new_sip_state)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "SIP state changed to: %d", new_sip_state);

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_state.sip_state = new_sip_state;
        g_system_state.sip_registered = (new_sip_state == SIP_STATE_REGISTERED ||
                                        new_sip_state == SIP_STATE_CALLING ||
                                        new_sip_state == SIP_STATE_CONNECTED);

        switch (new_sip_state) {
            case SIP_STATE_REGISTERED:
                if (g_system_state.app_state == APP_STATE_INITIALIZING) {
                    app_controller_transition_state(APP_STATE_IDLE);
                }
                break;

            case SIP_STATE_CALLING:
                if (g_system_state.app_state == APP_STATE_IDLE) {
                    app_controller_transition_state(APP_STATE_CALLING);
                }
                break;

            case SIP_STATE_CONNECTED:
                if (g_system_state.app_state == APP_STATE_CALLING) {
                    app_controller_transition_state(APP_STATE_CONNECTED);
                }
                break;

            case SIP_STATE_IDLE:
                if (g_system_state.app_state == APP_STATE_CALLING ||
                    g_system_state.app_state == APP_STATE_CONNECTED) {
                    app_controller_transition_state(APP_STATE_IDLE);
                }
                break;

            case SIP_STATE_ERROR:
                app_controller_handle_error(-1, "SIP error occurred");
                break;

            default:
                break;
        }

        xSemaphoreGive(g_state_mutex);
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_controller_handle_relay_state_change(relay_id_t relay, relay_state_t new_state)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Relay %d state changed to: %s", relay, 
             new_state == RELAY_STATE_ON ? "ON" : "OFF");

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (relay == RELAY_DOOR) {
            g_system_state.door_relay_state = new_state;
        } else if (relay == RELAY_LIGHT) {
            g_system_state.light_relay_state = new_state;
        }
        xSemaphoreGive(g_state_mutex);
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_controller_handle_error(int error_code, const char *error_message)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    error_severity_t severity = ERROR_SEVERITY_ERROR;
    if (error_code < 0) {
        severity = ERROR_SEVERITY_CRITICAL;
    }
    
    uint32_t error_id = ERROR_REPORT_SYSTEM(severity, "app_controller", error_code, 
                                           "%s", error_message ? error_message : "Unknown error");

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_state.error_count++;
        
        if (error_message) {
            strncpy(g_system_state.last_error, error_message, sizeof(g_system_state.last_error) - 1);
            g_system_state.last_error[sizeof(g_system_state.last_error) - 1] = '\0';
        } else {
            snprintf(g_system_state.last_error, sizeof(g_system_state.last_error), 
                     "Error ID: %" PRIu32, error_id);
        }

        if (g_system_state.app_state != APP_STATE_ERROR) {
            app_controller_transition_state(APP_STATE_ERROR);
        }

        xSemaphoreGive(g_state_mutex);
    } else {
        ERROR_REPORT_SYSTEM(ERROR_SEVERITY_WARNING, "app_controller", ESP_ERR_TIMEOUT, 
                           "Failed to acquire state mutex for error handling");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t app_controller_update_config(const door_station_config_t *config)
{
    if (!g_controller_initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Configuration updated - restarting services");
    
    return ESP_OK;
}

const char* app_controller_get_state_string(app_state_t state)
{
    switch (state) {
        case APP_STATE_INITIALIZING: return "INITIALIZING";
        case APP_STATE_IDLE: return "IDLE";
        case APP_STATE_CALLING: return "CALLING";
        case APP_STATE_CONNECTED: return "CONNECTED";
        case APP_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool app_controller_is_system_ready(void)
{
    if (!g_controller_initialized) {
        return false;
    }

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        bool ready = (g_system_state.app_state == APP_STATE_IDLE ||
                     g_system_state.app_state == APP_STATE_CALLING ||
                     g_system_state.app_state == APP_STATE_CONNECTED);
        xSemaphoreGive(g_state_mutex);
        return ready;
    }

    return false;
}

uint32_t app_controller_get_uptime(void)
{
    if (!g_controller_initialized) {
        return 0;
    }

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint32_t uptime = g_system_state.uptime_seconds;
        xSemaphoreGive(g_state_mutex);
        return uptime;
    }

    return 0;
}

esp_err_t app_controller_reset_error_count(void)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        g_system_state.error_count = 0;
        memset(g_system_state.last_error, 0, sizeof(g_system_state.last_error));
        xSemaphoreGive(g_state_mutex);
        ESP_LOGI(TAG, "Error count reset");
    } else {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

static void app_controller_update_uptime(void)
{
    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint32_t current_time = esp_timer_get_time() / 1000000;
        g_system_state.uptime_seconds = current_time - g_init_time;
        xSemaphoreGive(g_state_mutex);
    }
}

static esp_err_t app_controller_transition_state(app_state_t new_state)
{
    if (xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        app_state_t old_state = g_system_state.app_state;
        
        if (old_state != new_state) {
            g_system_state.app_state = new_state;
            app_controller_log_state_transition(old_state, new_state);
        }
        
        xSemaphoreGive(g_state_mutex);
        return ESP_OK;
    }
    
    return ESP_ERR_TIMEOUT;
}

static void app_controller_log_state_transition(app_state_t old_state, app_state_t new_state)
{
    ESP_LOGI(TAG, "State transition: %s -> %s", 
             app_controller_get_state_string(old_state),
             app_controller_get_state_string(new_state));
}

esp_err_t app_controller_stop(void)
{
    if (!g_controller_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping application controller");

    app_controller_transition_state(APP_STATE_ERROR);

    if (g_app_event_loop != NULL) {
        esp_event_loop_delete(g_app_event_loop);
        g_app_event_loop = NULL;
    }

    if (g_state_mutex != NULL) {
        vSemaphoreDelete(g_state_mutex);
        g_state_mutex = NULL;
    }

    g_controller_initialized = false;
    ESP_LOGI(TAG, "Application controller stopped");

    return ESP_OK;
}