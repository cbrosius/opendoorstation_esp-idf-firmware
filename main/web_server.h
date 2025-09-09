#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "io_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the web server
 * 
 * @param port Port number to listen on
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t web_server_init(uint16_t port);

/**
 * @brief Stop and cleanup the web server
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t web_server_stop(void);

/**
 * @brief Check if web server is running
 * 
 * @return true if server is running, false otherwise
 */
bool web_server_is_running(void);

/**
 * @brief Broadcast relay status update to SSE clients
 * 
 * @param relay Relay that changed state
 * @param state New relay state
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t web_server_broadcast_relay_status(relay_id_t relay, relay_state_t state);

/**
 * @brief Log the web interface URL (useful when WiFi connects)
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t web_server_log_url(void);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H