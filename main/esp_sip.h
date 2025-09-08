#ifndef ESP_SIP_H
#define ESP_SIP_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SIP client handle
 */
typedef struct esp_sip_client* esp_sip_client_handle_t;

/**
 * @brief SIP event types
 */
typedef enum {
    ESP_SIP_EVENT_REGISTERED,
    ESP_SIP_EVENT_REGISTRATION_FAILED,
    ESP_SIP_EVENT_CALL_STARTED,
    ESP_SIP_EVENT_CALL_CONNECTED,
    ESP_SIP_EVENT_CALL_ENDED,
    ESP_SIP_EVENT_CALL_FAILED,
    ESP_SIP_EVENT_DTMF_RECEIVED
} esp_sip_event_t;

/**
 * @brief SIP configuration
 */
typedef struct {
    char *uri;
    char *username;
    char *password;
    char *server_uri;
    uint16_t port;
    uint32_t registration_timeout_sec;
    uint32_t call_timeout_sec;
} esp_sip_config_t;

/**
 * @brief SIP event data
 */
typedef struct {
    esp_sip_event_t event;
    union {
        struct {
            char digit;
        } dtmf;
        struct {
            int code;
            char *message;
        } error;
    } data;
} esp_sip_event_data_t;

/**
 * @brief SIP event callback
 */
typedef void (*esp_sip_event_callback_t)(esp_sip_event_data_t *event_data, void *user_data);

/**
 * @brief Initialize SIP client
 */
esp_err_t esp_sip_init(esp_sip_config_t *config, esp_sip_event_callback_t callback, void *user_data, esp_sip_client_handle_t *client);

/**
 * @brief Start SIP client
 */
esp_err_t esp_sip_start(esp_sip_client_handle_t client);

/**
 * @brief Stop SIP client
 */
esp_err_t esp_sip_stop(esp_sip_client_handle_t client);

/**
 * @brief Make a call
 */
esp_err_t esp_sip_call(esp_sip_client_handle_t client, const char *uri);

/**
 * @brief End call
 */
esp_err_t esp_sip_hangup(esp_sip_client_handle_t client);

/**
 * @brief Destroy SIP client
 */
esp_err_t esp_sip_destroy(esp_sip_client_handle_t client);

#ifdef __cplusplus
}
#endif

#endif // ESP_SIP_H