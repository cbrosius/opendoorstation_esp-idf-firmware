#ifndef SIP_MANAGER_H
#define SIP_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SIP manager states
 */
typedef enum {
    SIP_STATE_IDLE,          ///< SIP manager not initialized or stopped
    SIP_STATE_REGISTERING,   ///< Attempting to register with SIP server
    SIP_STATE_REGISTERED,    ///< Successfully registered with SIP server
    SIP_STATE_CALLING,       ///< Outgoing call in progress
    SIP_STATE_CONNECTED,     ///< Call connected and active
    SIP_STATE_ERROR          ///< Error state
} sip_state_t;

/**
 * @brief SIP configuration structure
 */
typedef struct {
    char user[32];           ///< SIP username
    char domain[64];         ///< SIP domain/server
    char password[64];       ///< SIP password
    char callee[64];         ///< Default callee URI
    uint16_t port;           ///< SIP server port (default 5060)
    uint32_t registration_timeout; ///< Registration timeout in seconds
    uint32_t call_timeout;   ///< Call timeout in seconds
} sip_config_t;

/**
 * @brief DTMF callback function type
 * 
 * @param digit DTMF digit received ('0'-'9', '*', '#')
 * @param user_data User data passed during callback registration
 */
typedef void (*dtmf_callback_t)(char digit, void *user_data);

/**
 * @brief SIP event types
 */
typedef enum {
    SIP_EVENT_REGISTERED,      ///< SIP client successfully registered
    SIP_EVENT_REGISTRATION_FAILED, ///< SIP registration failed
    SIP_EVENT_CALL_STARTED,    ///< Outgoing call initiated
    SIP_EVENT_CALL_CONNECTED,  ///< Call answered by remote party
    SIP_EVENT_CALL_ENDED,      ///< Call terminated
    SIP_EVENT_CALL_FAILED,     ///< Call failed to connect
    SIP_EVENT_DTMF_RECEIVED    ///< DTMF tone received
} sip_event_type_t;

/**
 * @brief SIP event data structure
 */
typedef struct {
    sip_event_type_t event_type;
    union {
        struct {
            char digit;
            uint32_t timestamp;
        } dtmf;
        struct {
            int error_code;
            char error_message[128];
        } error;
    } data;
} sip_event_data_t;

/**
 * @brief Initialize SIP manager
 * 
 * @param config SIP configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_init(const sip_config_t *config);

/**
 * @brief Start SIP manager and register with server
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_start(void);

/**
 * @brief Stop SIP manager and unregister
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_stop(void);

/**
 * @brief Start an outgoing call
 * 
 * @param uri SIP URI to call (if NULL, uses configured callee)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_start_call(const char *uri);

/**
 * @brief End the current call
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_end_call(void);

/**
 * @brief Get current SIP manager state
 * 
 * @return Current SIP state
 */
sip_state_t sip_manager_get_state(void);

/**
 * @brief Register DTMF callback
 * 
 * @param callback Callback function to call when DTMF is received
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_register_dtmf_callback(dtmf_callback_t callback, void *user_data);

/**
 * @brief Check if a call is currently active
 * 
 * @return true if call is active, false otherwise
 */
bool sip_manager_is_call_active(void);

/**
 * @brief Get call duration in seconds
 * 
 * @return Call duration in seconds, 0 if no active call
 */
uint32_t sip_manager_get_call_duration(void);

/**
 * @brief Update SIP configuration
 * 
 * @param config New SIP configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_update_config(const sip_config_t *config);

/**
 * @brief Get call statistics
 * 
 * @param stats Pointer to structure to fill with call statistics
 * @return ESP_OK on success, error code otherwise
 */
typedef struct {
    uint32_t total_calls_made;
    uint32_t successful_calls;
    uint32_t failed_calls;
    uint32_t total_call_duration;
    uint32_t current_call_duration;
    uint32_t last_call_end_reason;
} sip_call_stats_t;

esp_err_t sip_manager_get_call_stats(sip_call_stats_t *stats);

/**
 * @brief Reset call statistics
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_reset_call_stats(void);

/**
 * @brief DTMF command types
 */
typedef enum {
    DTMF_CMD_NONE = 0,           ///< No command
    DTMF_CMD_DOOR_OPEN,          ///< Open door relay
    DTMF_CMD_DOOR_CLOSE,         ///< Close door relay (if applicable)
    DTMF_CMD_STATUS_REQUEST,     ///< Request system status
    DTMF_CMD_HANGUP,             ///< Hang up call
    DTMF_CMD_CUSTOM             ///< Custom user-defined command
} dtmf_command_t;

/**
 * @brief DTMF command mapping structure
 */
typedef struct {
    char digit;                  ///< DTMF digit ('0'-'9', '*', '#')
    dtmf_command_t command;      ///< Command to execute
    uint32_t param;              ///< Optional parameter for command
    bool enabled;                ///< Whether this mapping is enabled
} dtmf_command_mapping_t;

/**
 * @brief DTMF command callback function type
 * 
 * @param command Command to execute
 * @param param Optional parameter
 * @param user_data User data passed during callback registration
 */
typedef void (*dtmf_command_callback_t)(dtmf_command_t command, uint32_t param, void *user_data);

/**
 * @brief Configure DTMF command mappings
 * 
 * @param mappings Array of command mappings
 * @param count Number of mappings in array
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_configure_dtmf_commands(const dtmf_command_mapping_t *mappings, size_t count);

/**
 * @brief Register DTMF command callback
 * 
 * @param callback Callback function to call when DTMF command is recognized
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_register_dtmf_command_callback(dtmf_command_callback_t callback, void *user_data);

/**
 * @brief Get current DTMF command mappings
 * 
 * @param mappings Buffer to store mappings
 * @param max_count Maximum number of mappings to store
 * @param actual_count Pointer to store actual number of mappings
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_get_dtmf_commands(dtmf_command_mapping_t *mappings, size_t max_count, size_t *actual_count);

/**
 * @brief Enable or disable DTMF command processing
 * 
 * @param enabled True to enable, false to disable
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_manager_set_dtmf_processing_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // SIP_MANAGER_H