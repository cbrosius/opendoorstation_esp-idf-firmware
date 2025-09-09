#include "error_handler.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdarg.h>

static const char *TAG = "error_handler";

// Maximum number of errors to store in history
#define MAX_ERROR_HISTORY 50

// Error storage
static error_info_t g_error_history[MAX_ERROR_HISTORY];
static uint32_t g_error_count = 0;
static uint32_t g_next_error_id = 1;
static uint32_t g_history_index = 0;
static error_stats_t g_error_stats = {0};
static SemaphoreHandle_t g_error_mutex = NULL;
static bool g_error_handler_initialized = false;

// Error callback
static error_callback_t g_error_callback = NULL;
static void *g_callback_user_data = NULL;

// Recovery actions per category
static error_recovery_t g_category_recovery[ERROR_CATEGORY_MAX] = {
    ERROR_RECOVERY_NONE,        // SYSTEM
    ERROR_RECOVERY_RETRY,       // NETWORK
    ERROR_RECOVERY_RETRY,       // SIP
    ERROR_RECOVERY_NONE,        // HARDWARE
    ERROR_RECOVERY_NONE,        // CONFIG
    ERROR_RECOVERY_RESTART_SERVICE // WEB
};

// Forward declarations
static void error_handler_update_stats(const error_info_t *error_info);
static uint32_t error_handler_find_duplicate(error_category_t category, const char *component, const char *message);

esp_err_t error_handler_init(void)
{
    if (g_error_handler_initialized) {
        ESP_LOGW(TAG, "Error handler already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing error handler");

    // Create mutex for thread safety
    g_error_mutex = xSemaphoreCreateMutex();
    if (g_error_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create error handler mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize error history
    memset(g_error_history, 0, sizeof(g_error_history));
    memset(&g_error_stats, 0, sizeof(g_error_stats));
    
    g_error_count = 0;
    g_next_error_id = 1;
    g_history_index = 0;

    g_error_handler_initialized = true;
    ESP_LOGI(TAG, "Error handler initialized successfully");

    return ESP_OK;
}

uint32_t error_handler_report(error_category_t category, 
                             error_severity_t severity,
                             const char *component,
                             int esp_error_code,
                             const char *format, ...)
{
    if (!g_error_handler_initialized) {
        ESP_LOGE(TAG, "Error handler not initialized");
        return 0;
    }

    if (category >= ERROR_CATEGORY_MAX || component == NULL || format == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for error reporting");
        return 0;
    }

    // Format error message
    char message[128];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    uint32_t error_id = 0;

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Check for duplicate error
        uint32_t duplicate_id = error_handler_find_duplicate(category, component, message);
        
        if (duplicate_id > 0) {
            // Update existing error
            for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
                if (g_error_history[i].error_id == duplicate_id) {
                    g_error_history[i].count++;
                    g_error_history[i].last_occurrence = esp_timer_get_time() / 1000000;
                    error_id = duplicate_id;
                    break;
                }
            }
        } else {
            // Create new error entry
            error_info_t *error_info = &g_error_history[g_history_index];
            
            error_info->error_id = g_next_error_id++;
            error_info->category = category;
            error_info->severity = severity;
            error_info->recovery = g_category_recovery[category];
            error_info->esp_error_code = esp_error_code;
            error_info->timestamp = esp_timer_get_time() / 1000000;
            error_info->count = 1;
            error_info->last_occurrence = error_info->timestamp;
            
            strncpy(error_info->component, component, sizeof(error_info->component) - 1);
            error_info->component[sizeof(error_info->component) - 1] = '\0';
            
            strncpy(error_info->message, message, sizeof(error_info->message) - 1);
            error_info->message[sizeof(error_info->message) - 1] = '\0';
            
            error_id = error_info->error_id;
            
            // Update statistics
            error_handler_update_stats(error_info);
            
            // Move to next history slot (circular buffer)
            g_history_index = (g_history_index + 1) % MAX_ERROR_HISTORY;
            if (g_error_count < MAX_ERROR_HISTORY) {
                g_error_count++;
            }
            
            // Call error callback if registered
            if (g_error_callback != NULL) {
                g_error_callback(error_info, g_callback_user_data);
            }
        }
        
        xSemaphoreGive(g_error_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to acquire error handler mutex");
        return 0;
    }

    // Log the error using ESP-IDF logging
    const char *severity_str = error_handler_get_severity_string(severity);
    const char *category_str = error_handler_get_category_string(category);
    
    switch (severity) {
        case ERROR_SEVERITY_INFO:
            ESP_LOGI(TAG, "[%s/%s] %s: %s (ID: %" PRIu32 ")", 
                     category_str, component, severity_str, message, error_id);
            break;
        case ERROR_SEVERITY_WARNING:
            ESP_LOGW(TAG, "[%s/%s] %s: %s (ID: %" PRIu32 ")", 
                     category_str, component, severity_str, message, error_id);
            break;
        case ERROR_SEVERITY_ERROR:
            ESP_LOGE(TAG, "[%s/%s] %s: %s (ID: %" PRIu32 ")", 
                     category_str, component, severity_str, message, error_id);
            break;
        case ERROR_SEVERITY_CRITICAL:
            ESP_LOGE(TAG, "[%s/%s] CRITICAL: %s (ID: %" PRIu32 ")", 
                     category_str, component, message, error_id);
            break;
    }

    if (esp_error_code != ESP_OK) {
        ESP_LOGE(TAG, "ESP Error Code: %s (%d)", esp_err_to_name(esp_error_code), esp_error_code);
    }

    return error_id;
}

esp_err_t error_handler_get_error_info(uint32_t error_id, error_info_t *error_info)
{
    if (!g_error_handler_initialized || error_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < g_error_count; i++) {
            if (g_error_history[i].error_id == error_id) {
                memcpy(error_info, &g_error_history[i], sizeof(error_info_t));
                xSemaphoreGive(g_error_mutex);
                return ESP_OK;
            }
        }
        xSemaphoreGive(g_error_mutex);
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t error_handler_get_stats(error_stats_t *stats)
{
    if (!g_error_handler_initialized || stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(stats, &g_error_stats, sizeof(error_stats_t));
        xSemaphoreGive(g_error_mutex);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t error_handler_clear_history(void)
{
    if (!g_error_handler_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(g_error_history, 0, sizeof(g_error_history));
        memset(&g_error_stats, 0, sizeof(g_error_stats));
        g_error_count = 0;
        g_next_error_id = 1;
        g_history_index = 0;
        xSemaphoreGive(g_error_mutex);
        ESP_LOGI(TAG, "Error history cleared");
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t error_handler_register_callback(error_callback_t callback, void *user_data)
{
    if (!g_error_handler_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    g_error_callback = callback;
    g_callback_user_data = user_data;
    
    ESP_LOGI(TAG, "Error callback registered");
    return ESP_OK;
}

const char* error_handler_get_category_string(error_category_t category)
{
    switch (category) {
        case ERROR_CATEGORY_SYSTEM: return "SYSTEM";
        case ERROR_CATEGORY_NETWORK: return "NETWORK";
        case ERROR_CATEGORY_SIP: return "SIP";
        case ERROR_CATEGORY_HARDWARE: return "HARDWARE";
        case ERROR_CATEGORY_CONFIG: return "CONFIG";
        case ERROR_CATEGORY_WEB: return "WEB";
        default: return "UNKNOWN";
    }
}

const char* error_handler_get_severity_string(error_severity_t severity)
{
    switch (severity) {
        case ERROR_SEVERITY_INFO: return "INFO";
        case ERROR_SEVERITY_WARNING: return "WARNING";
        case ERROR_SEVERITY_ERROR: return "ERROR";
        case ERROR_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

const char* error_handler_get_recovery_string(error_recovery_t recovery)
{
    switch (recovery) {
        case ERROR_RECOVERY_NONE: return "NONE";
        case ERROR_RECOVERY_RETRY: return "RETRY";
        case ERROR_RECOVERY_RESTART_SERVICE: return "RESTART_SERVICE";
        case ERROR_RECOVERY_FACTORY_RESET: return "FACTORY_RESET";
        case ERROR_RECOVERY_REBOOT: return "REBOOT";
        default: return "UNKNOWN";
    }
}

bool error_handler_has_critical_errors(void)
{
    if (!g_error_handler_initialized) {
        return false;
    }

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        bool has_critical = g_error_stats.critical_errors > 0;
        xSemaphoreGive(g_error_mutex);
        return has_critical;
    }

    return false;
}

uint32_t error_handler_get_last_error_id(void)
{
    if (!g_error_handler_initialized) {
        return 0;
    }

    if (xSemaphoreTake(g_error_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint32_t last_id = g_error_stats.last_error_id;
        xSemaphoreGive(g_error_mutex);
        return last_id;
    }

    return 0;
}

esp_err_t error_handler_set_category_recovery(error_category_t category, error_recovery_t recovery)
{
    if (!g_error_handler_initialized || category >= ERROR_CATEGORY_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    g_category_recovery[category] = recovery;
    ESP_LOGI(TAG, "Recovery action for %s set to %s", 
             error_handler_get_category_string(category),
             error_handler_get_recovery_string(recovery));

    return ESP_OK;
}

// Private functions

static void error_handler_update_stats(const error_info_t *error_info)
{
    g_error_stats.total_errors++;
    g_error_stats.errors_by_category[error_info->category]++;
    g_error_stats.last_error_id = error_info->error_id;
    g_error_stats.uptime_at_last_error = error_info->timestamp;

    if (error_info->severity == ERROR_SEVERITY_CRITICAL) {
        g_error_stats.critical_errors++;
    }
}

static uint32_t error_handler_find_duplicate(error_category_t category, const char *component, const char *message)
{
    for (int i = 0; i < g_error_count; i++) {
        if (g_error_history[i].category == category &&
            strcmp(g_error_history[i].component, component) == 0 &&
            strcmp(g_error_history[i].message, message) == 0) {
            return g_error_history[i].error_id;
        }
    }
    return 0;
}