#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi connection states
 */
typedef enum {
    WIFI_STATE_DISCONNECTED,    ///< WiFi not connected
    WIFI_STATE_CONNECTING,      ///< WiFi connection in progress
    WIFI_STATE_CONNECTED,       ///< WiFi connected with IP address
    WIFI_STATE_ERROR            ///< WiFi connection error
} wifi_state_t;

/**
 * @brief WiFi connection information
 */
typedef struct {
    wifi_state_t state;         ///< Current WiFi state
    char ssid[32];              ///< Connected SSID
    char ip_address[16];        ///< Assigned IP address (e.g., "192.168.1.100")
    char gateway[16];           ///< Gateway IP address
    char netmask[16];           ///< Network mask
    int8_t rssi;                ///< Signal strength (dBm)
    uint32_t connect_time;      ///< Connection timestamp
    uint32_t retry_count;       ///< Number of connection retries
} wifi_info_t;

/**
 * @brief WiFi event callback function type
 * 
 * @param state New WiFi state
 * @param info WiFi connection information
 * @param user_data User data passed during callback registration
 */
typedef void (*wifi_event_callback_t)(wifi_state_t state, const wifi_info_t *info, void *user_data);

/**
 * @brief Initialize WiFi manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Connect to WiFi network
 * 
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Disconnect from WiFi network
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi connection information
 * 
 * @param info Pointer to wifi_info_t structure to fill
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_get_info(wifi_info_t *info);

/**
 * @brief Register WiFi event callback
 * 
 * @param callback Callback function to call on WiFi events
 * @param user_data User data to pass to callback
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_register_callback(wifi_event_callback_t callback, void *user_data);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get current IP address as string
 * 
 * @return IP address string or NULL if not connected
 */
const char* wifi_manager_get_ip_address(void);

/**
 * @brief Get WiFi state as string
 * 
 * @param state WiFi state
 * @return String representation of state
 */
const char* wifi_manager_get_state_string(wifi_state_t state);

/**
 * @brief Stop WiFi manager and cleanup resources
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_stop(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H