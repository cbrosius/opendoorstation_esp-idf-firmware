#ifndef MOCK_ESP_SIP_H
#define MOCK_ESP_SIP_H

#include "esp_sip.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock control structure for esp_sip
 */
typedef struct {
    bool init_should_fail;
    bool start_should_fail;
    bool call_should_fail;
    bool hangup_should_fail;
    esp_sip_event_callback_t registered_callback;
    void *callback_user_data;
    esp_sip_client_handle_t last_client;
    char last_call_uri[128];
    int init_call_count;
    int start_call_count;
    int call_call_count;
    int hangup_call_count;
    int destroy_call_count;
} mock_esp_sip_control_t;

/**
 * @brief Get mock control structure
 */
mock_esp_sip_control_t* mock_esp_sip_get_control(void);

/**
 * @brief Reset mock state
 */
void mock_esp_sip_reset(void);

/**
 * @brief Simulate SIP event
 */
void mock_esp_sip_simulate_event(esp_sip_event_t event, void *event_data);

/**
 * @brief Simulate DTMF event
 */
void mock_esp_sip_simulate_dtmf(char digit);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_SIP_H