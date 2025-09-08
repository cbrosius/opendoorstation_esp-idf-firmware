#ifndef SIP_IO_INTEGRATION_H
#define SIP_IO_INTEGRATION_H

#include "esp_err.h"
#include "sip_manager.h"
#include "io_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SIP-IO integration configuration
 */
typedef struct {
    uint32_t door_pulse_duration_ms;    ///< Duration for door relay pulse
    bool auto_hangup_after_door_open;   ///< Automatically hang up after door opens
    uint32_t hangup_delay_ms;           ///< Delay before auto hangup (if enabled)
    bool status_feedback_enabled;       ///< Enable status feedback via SIP events
} sip_io_config_t;

/**
 * @brief Initialize SIP-IO integration
 * 
 * Sets up the integration between SIP manager and I/O manager.
 * Registers DTMF command callbacks and configures default mappings.
 * 
 * @param config Integration configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_io_integration_init(const sip_io_config_t *config);

/**
 * @brief Start SIP-IO integration
 * 
 * Enables the integration between SIP and I/O systems.
 * Must be called after both SIP manager and I/O manager are initialized.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_io_integration_start(void);

/**
 * @brief Stop SIP-IO integration
 * 
 * Disables the integration between SIP and I/O systems.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_io_integration_stop(void);

/**
 * @brief Update integration configuration
 * 
 * @param config New integration configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_io_integration_update_config(const sip_io_config_t *config);

/**
 * @brief Get current integration configuration
 * 
 * @param config Pointer to store current configuration
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sip_io_integration_get_config(sip_io_config_t *config);

/**
 * @brief Check if integration is active
 * 
 * @return true if integration is running, false otherwise
 */
bool sip_io_integration_is_active(void);

#ifdef __cplusplus
}
#endif

#endif // SIP_IO_INTEGRATION_H