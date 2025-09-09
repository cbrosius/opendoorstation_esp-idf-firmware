#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "esp_err.h"
#include "esp_event.h"
#include "sip_manager.h"
#include "io_manager.h"
#include "config_manager.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application states
 */
typedef enum {
    APP_STATE_INITIALIZING,     ///< System is starting up
    APP_STATE_IDLE,             ///< System ready, waiting for events
    APP_STATE_CALLING,          ///< SIP call in progress
    APP_STATE_CONNECTED,        ///< Call connected, ready for DTMF
    APP_STATE_ERROR             ///< System error state
} app_state_t;

/**
 * @brief System state structure
 */
typedef struct {
    app_state_t app_state;              ///< Current application state
    sip_state_t sip_state;              ///< Current SIP state
    relay_state_t door_relay_state;     ///< Door relay state
    relay_state_t light_relay_state;    ///< Light relay state
    bool button_pressed;                ///< Current button state
    uint32_t call_start_time;           ///< Timestamp when call started
    uint32_t last_dtmf_time;            ///< Timestamp of last DTMF received
    char last_error[128];               ///< Last error message
    uint32_t error_count;               ///< Number of errors since startup
    bool sip_registered;                ///< SIP registration status
    uint32_t uptime_seconds;            ///< System uptime in seconds
} system_state_t;

/**
 * @brief Application event types
 */
typedef enum {
    APP_EVENT_SYSTEM_READY,         ///< System initialization complete
    APP_EVENT_BUTTON_PRESSED,       ///< Physical or virtual button pressed
    APP_EVENT_CALL_INITIATED,       ///< SIP call started
    APP_EVENT_CALL_CONNECTED,       ///< SIP call connected
    APP_EVENT_CALL_ENDED,           ///< SIP call ended
    APP_EVENT_DTMF_RECEIVED,        ///< DTMF tone received
    APP_EVENT_RELAY_OPERATED,       ///< Relay operation completed
    APP_EVENT_ERROR_OCCURRED,       ///< System error occurred
    APP_EVENT_CONFIG_UPDATED        ///< Configuration updated
} app_event_type_t;

/**
 * @brief Application event data
 */
typedef struct {
    app_event_type_t event_type;
    union {
        struct {
            char digit;
            uint32_t timestamp;
        } dtmf;
        struct {
            relay_id_t relay;
            relay_state_t state;
        } relay;
        struct {
            int error_code;
            char message[128];
        } error;
    } data;
} app_event_data_t;

/**
 * @brief Initialize application controller
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_init(void);

/**
 * @brief Start main application event loop
 * 
 * This function starts the main event loop and does not return.
 * It should be called from app_main() after all components are initialized.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_start_event_loop(void);

/**
 * @brief Stop application controller and cleanup resources
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_stop(void);

/**
 * @brief Get current system state
 * 
 * @param state Pointer to system_state_t structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_get_system_state(system_state_t *state);

/**
 * @brief Set application state
 * 
 * @param new_state New application state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_set_app_state(app_state_t new_state);

/**
 * @brief Handle button press event
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_handle_button_press(void);

/**
 * @brief Handle DTMF digit received
 * 
 * @param digit DTMF digit ('0'-'9', '*', '#')
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_handle_dtmf(char digit);

/**
 * @brief Handle SIP state change
 * 
 * @param new_sip_state New SIP state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_handle_sip_state_change(sip_state_t new_sip_state);

/**
 * @brief Handle relay state change
 * 
 * @param relay Relay that changed
 * @param new_state New relay state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_handle_relay_state_change(relay_id_t relay, relay_state_t new_state);

/**
 * @brief Handle system error
 * 
 * @param error_code Error code
 * @param error_message Error message
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_handle_error(int error_code, const char *error_message);

/**
 * @brief Update configuration
 * 
 * @param config New configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_update_config(const door_station_config_t *config);

/**
 * @brief Get application state string
 * 
 * @param state Application state
 * @return String representation of state
 */
const char* app_controller_get_state_string(app_state_t state);

/**
 * @brief Check if system is ready for operations
 * 
 * @return true if system is ready, false otherwise
 */
bool app_controller_is_system_ready(void);

/**
 * @brief Get system uptime in seconds
 * 
 * @return System uptime in seconds
 */
uint32_t app_controller_get_uptime(void);

/**
 * @brief Reset error count
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_controller_reset_error_count(void);

#ifdef __cplusplus
}
#endif

#endif // APP_CONTROLLER_H