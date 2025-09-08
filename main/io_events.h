#ifndef IO_EVENTS_H
#define IO_EVENTS_H

#include "esp_event.h"
#include "io_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I/O event declarations
 */
ESP_EVENT_DECLARE_BASE(IO_EVENTS);

/**
 * @brief I/O event types
 */
typedef enum {
    IO_EVENT_BUTTON_PRESSED,        /**< Button was pressed */
    IO_EVENT_BUTTON_RELEASED,       /**< Button was released */
    IO_EVENT_RELAY_STATE_CHANGED,   /**< Relay state changed */
} io_event_id_t;

/**
 * @brief Button event data
 */
typedef struct {
    bool pressed;           /**< True if pressed, false if released */
    uint32_t timestamp;     /**< Timestamp of the event */
} io_button_event_data_t;

/**
 * @brief Relay state change event data
 */
typedef struct {
    relay_id_t relay;           /**< Which relay changed */
    relay_state_t old_state;    /**< Previous state */
    relay_state_t new_state;    /**< New state */
    uint32_t timestamp;         /**< Timestamp of the event */
} io_relay_event_data_t;

/**
 * @brief Initialize I/O event system
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_events_init(void);

/**
 * @brief Publish button event
 * 
 * @param pressed True if button pressed, false if released
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_events_publish_button(bool pressed);

/**
 * @brief Publish relay state change event
 * 
 * @param relay Relay that changed
 * @param old_state Previous state
 * @param new_state New state
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t io_events_publish_relay_state_change(relay_id_t relay, 
                                               relay_state_t old_state, 
                                               relay_state_t new_state);

#ifdef __cplusplus
}
#endif

#endif // IO_EVENTS_H