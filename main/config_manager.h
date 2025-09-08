#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for the door station
 */
typedef struct {
    char wifi_ssid[32];              ///< Wi-Fi SSID (1-31 characters)
    char wifi_password[64];          ///< Wi-Fi password
    char sip_user[32];               ///< SIP username (3-31 characters, alphanumeric and underscore)
    char sip_domain[64];             ///< SIP domain (hostname or IP address)
    char sip_password[64];           ///< SIP password
    char sip_callee[64];             ///< SIP callee URI
    uint16_t web_port;               ///< Web server port (1024-65535)
    uint32_t door_pulse_duration;    ///< Door relay pulse duration in ms (500-10000)
} door_station_config_t;

/**
 * @brief Validation error codes
 */
typedef enum {
    CONFIG_VALIDATION_OK = 0,
    CONFIG_VALIDATION_WIFI_SSID_INVALID,
    CONFIG_VALIDATION_WIFI_SSID_TOO_LONG,
    CONFIG_VALIDATION_SIP_USER_INVALID,
    CONFIG_VALIDATION_SIP_USER_TOO_SHORT,
    CONFIG_VALIDATION_SIP_USER_TOO_LONG,
    CONFIG_VALIDATION_SIP_DOMAIN_INVALID,
    CONFIG_VALIDATION_SIP_CALLEE_INVALID,
    CONFIG_VALIDATION_WEB_PORT_INVALID,
    CONFIG_VALIDATION_DOOR_PULSE_INVALID
} config_validation_error_t;

/**
 * @brief Initialize configuration manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_init(void);

/**
 * @brief Load configuration from storage
 * 
 * @param config Pointer to configuration structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_load(door_station_config_t *config);

/**
 * @brief Save configuration to storage
 * 
 * @param config Pointer to configuration structure to save
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_save(const door_station_config_t *config);

/**
 * @brief Validate configuration parameters
 * 
 * @param config Pointer to configuration structure to validate
 * @return CONFIG_VALIDATION_OK if valid, specific error code otherwise
 */
config_validation_error_t config_manager_validate(const door_station_config_t *config);

/**
 * @brief Reset configuration to factory defaults
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t config_manager_factory_reset(void);

/**
 * @brief Get default configuration values
 * 
 * @param config Pointer to configuration structure to fill with defaults
 */
void config_manager_get_defaults(door_station_config_t *config);

/**
 * @brief Get validation error message
 * 
 * @param error Validation error code
 * @return Human-readable error message
 */
const char* config_manager_get_validation_error_message(config_validation_error_t error);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_MANAGER_H