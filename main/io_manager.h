#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Relay identifiers
 */
typedef enum {
    RELAY_DOOR = 0,     /**< Door relay */
    RELAY_LIGHT = 1     /**< Light relay */
} relay_id_t;

/**
 * @brief Relay states
 */
typedef enum {
    RELAY_STATE_OFF = 0,    /**< Relay is off */
    RELAY_STATE_ON = 1      /**< Relay is on */
} relay_state_t;

/**
 * @brief Button callback function type
 * 
 * @param pressed True if button is pressed, false if released
 */
typedef void (*button_callback_t)(bool pressed);

/**
 * @brief Initialize the I/O manager
 * 
 * Configures GPIO pins for button input and relay outputs.
 * Sets up button debouncing and relay safety features.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_manager_init(void);

/**
 * @brief Pulse a relay for a specified duration
 * 
 * Turns the relay on for the specified duration, then turns it off.
 * Implements safety protection to prevent multiple pulses within 5 seconds.
 * 
 * @param relay Relay to pulse
 * @param duration_ms Duration to keep relay on (in milliseconds)
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if protection active
 */
esp_err_t io_manager_pulse_relay(relay_id_t relay, uint32_t duration_ms);

/**
 * @brief Toggle a relay state
 * 
 * Changes relay from off to on or from on to off.
 * 
 * @param relay Relay to toggle
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_manager_toggle_relay(relay_id_t relay);

/**
 * @brief Get current relay state
 * 
 * @param relay Relay to query
 * @return Current relay state
 */
relay_state_t io_manager_get_relay_state(relay_id_t relay);

/**
 * @brief Register callback for button events
 * 
 * @param callback Function to call when button state changes
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_manager_register_button_callback(button_callback_t callback);

/**
 * @brief Trigger virtual button press (for web interface)
 * 
 * Simulates a physical button press for testing purposes.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_manager_virtual_button_press(void);

#ifdef __cplusplus
}
#endif

#endif // IO_MANAGER_H