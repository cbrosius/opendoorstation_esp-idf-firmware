#ifndef MOCK_ESP_NETIF_H
#define MOCK_ESP_NETIF_H

#include "esp_netif.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the netif mock
 */
void mock_esp_netif_init(void);

/**
 * @brief Cleanup the netif mock
 */
void mock_esp_netif_cleanup(void);

/**
 * @brief Reset the netif mock state
 */
void mock_esp_netif_reset(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_NETIF_H