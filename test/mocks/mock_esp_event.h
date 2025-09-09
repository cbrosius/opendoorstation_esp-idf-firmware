#ifndef MOCK_ESP_EVENT_H
#define MOCK_ESP_EVENT_H

#include "esp_event.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the event mock
 */
void mock_esp_event_init(void);

/**
 * @brief Cleanup the event mock
 */
void mock_esp_event_cleanup(void);

/**
 * @brief Reset the event mock state
 */
void mock_esp_event_reset(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_EVENT_H