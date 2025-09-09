#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include "esp_err.h"
#include "esp_log.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error categories for the door station system
 */
typedef enum {
    ERROR_CATEGORY_SYSTEM = 0,      ///< System-level errors
    ERROR_CATEGORY_NETWORK,         ///< Network and connectivity errors
    ERROR_CATEGORY_SIP,             ///< SIP protocol errors
    ERROR_CATEGORY_HARDWARE,        ///< Hardware and GPIO errors
    ERROR_CATEGORY_CONFIG,          ///< Configuration errors
    ERROR_CATEGORY_WEB,             ///< Web server errors
    ERROR_CATEGORY_MAX
} error_category_t;

/**
 * @brief Error severity levels
 */
typedef enum {
    ERROR_SEVERITY_INFO = 0,        ///< Informational (not really an error)
    ERROR_SEVERITY_WARNING,         ///< Warning - system can continue
    ERROR_SEVERITY_ERROR,           ///< Error - functionality impacted
    ERROR_SEVERITY_CRITICAL         ///< Critical - system may be unstable
} error_severity_t;

/**
 * @brief Error recovery actions
 */
typedef enum {
    ERROR_RECOVERY_NONE = 0,        ///< No recovery action needed
    ERROR_RECOVERY_RETRY,           ///< Retry the operation
    ERROR_RECOVERY_RESTART_SERVICE, ///< Restart affected service
    ERROR_RECOVERY_FACTORY_RESET,   ///< Factory reset required
    ERROR_RECOVERY_REBOOT           ///< System reboot required
} error_recovery_t;

/**
 * @brief Error information structure
 */
typedef struct {
    uint32_t error_id;              ///< Unique error identifier
    error_category_t category;      ///< Error category
    error_severity_t severity;      ///< Error severity
    error_recovery_t recovery;      ///< Suggested recovery action
    int esp_error_code;             ///< ESP-IDF error code (if applicable)
    char component[32];             ///< Component that reported the error
    char message[128];              ///< Human-readable error message
    uint32_t timestamp;             ///< Error timestamp
    uint32_t count;                 ///< Number of times this error occurred
    uint32_t last_occurrence;       ///< Last occurrence timestamp
} error_info_t;

/**
 * @brief Error statistics
 */
typedef struct {
    uint32_t total_errors;          ///< Total number of errors
    uint32_t errors_by_category[ERROR_CATEGORY_MAX]; ///< Errors per category
    uint32_t critical_errors;       ///< Number of critical errors
    uint32_t last_error_id;         ///< ID of last error
    uint32_t uptime_at_last_error;  ///< System uptime when last error occurred
} error_stats_t;

/**
 * @brief Error callback function type
 * 
 * @param error_info Pointer to error information
 * @param user_data User data passed during callback registration
 */
typedef void (*error_callback_t)(const error_info_t *error_info, void *user_data);

/**
 * @brief Initialize error handling system
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t error_handler_init(void);

/**
 * @brief Report an error to the error handling system
 * 
 * @param category Error category
 * @param severity Error severity
 * @param component Component name reporting the error
 * @param esp_error_code ESP-IDF error code (ESP_OK if not applicable)
 * @param format Printf-style format string for error message
 * @param ... Arguments for format string
 * @return Error ID assigned to this error
 */
uint32_t error_handler_report(error_category_t category, 
                             error_severity_t severity,
                             const char *component,
                             int esp_error_code,
                             const char *format, ...);

/**
 * @brief Get error information by ID
 * 
 * @param error_id Error ID to look up
 * @param error_info Pointer to structure to fill with error information
 * @return ESP_OK if found, ESP_ERR_NOT_FOUND otherwise
 */
esp_err_t error_handler_get_error_info(uint32_t error_id, error_info_t *error_info);

/**
 * @brief Get error statistics
 * 
 * @param stats Pointer to structure to fill with statistics
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t error_handler_get_stats(error_stats_t *stats);

/**
 * @brief Clear error history
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t error_handler_clear_history(void);

/**
 * @brief Register error callback
 * 
 * @param callback Callback function to call when errors occur
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t error_handler_register_callback(error_callback_t callback, void *user_data);

/**
 * @brief Get category name string
 * 
 * @param category Error category
 * @return String representation of category
 */
const char* error_handler_get_category_string(error_category_t category);

/**
 * @brief Get severity name string
 * 
 * @param severity Error severity
 * @return String representation of severity
 */
const char* error_handler_get_severity_string(error_severity_t severity);

/**
 * @brief Get recovery action string
 * 
 * @param recovery Recovery action
 * @return String representation of recovery action
 */
const char* error_handler_get_recovery_string(error_recovery_t recovery);

/**
 * @brief Check if system has critical errors
 * 
 * @return true if critical errors exist, false otherwise
 */
bool error_handler_has_critical_errors(void);

/**
 * @brief Get last error ID
 * 
 * @return Last error ID, 0 if no errors
 */
uint32_t error_handler_get_last_error_id(void);

/**
 * @brief Set error recovery action for a category
 * 
 * @param category Error category
 * @param recovery Recovery action to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t error_handler_set_category_recovery(error_category_t category, error_recovery_t recovery);

// Convenience macros for common error reporting
#define ERROR_REPORT_SYSTEM(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_SYSTEM, severity, component, esp_err, __VA_ARGS__)

#define ERROR_REPORT_NETWORK(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_NETWORK, severity, component, esp_err, __VA_ARGS__)

#define ERROR_REPORT_SIP(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_SIP, severity, component, esp_err, __VA_ARGS__)

#define ERROR_REPORT_HARDWARE(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_HARDWARE, severity, component, esp_err, __VA_ARGS__)

#define ERROR_REPORT_CONFIG(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_CONFIG, severity, component, esp_err, __VA_ARGS__)

#define ERROR_REPORT_WEB(severity, component, esp_err, ...) \
    error_handler_report(ERROR_CATEGORY_WEB, severity, component, esp_err, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLER_H