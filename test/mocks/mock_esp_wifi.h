#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H

#include "esp_wifi.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the WiFi mock
 */
void mock_esp_wifi_init(void);

/**
 * @brief Cleanup the WiFi mock
 */
void mock_esp_wifi_cleanup(void);

/**
 * @brief Get the last WiFi configuration that was set
 * 
 * @return Pointer to the last wifi_config_t that was passed to esp_wifi_set_config
 */
wifi_config_t* mock_esp_wifi_get_last_config(void);

/**
 * @brief Reset the WiFi mock state
 */
void mock_esp_wifi_reset(void);

/**
 * @brief Set the return value for esp_wifi_init
 * 
 * @param ret_val Return value to use
 */
void mock_esp_wifi_set_init_return(esp_err_t ret_val);

/**
 * @brief Set the return value for esp_wifi_set_config
 * 
 * @param ret_val Return value to use
 */
void mock_esp_wifi_set_config_return(esp_err_t ret_val);

/**
 * @brief Set the return value for esp_wifi_connect
 * 
 * @param ret_val Return value to use
 */
void mock_esp_wifi_set_connect_return(esp_err_t ret_val);

/**
 * @brief Get the number of times esp_wifi_connect was called
 * 
 * @return Number of calls
 */
int mock_esp_wifi_get_connect_call_count(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_WIFI_H