#include "mock_esp_sip.h"
#include <string.h>
#include <stdlib.h>

static mock_esp_sip_control_t mock_control = {0};

mock_esp_sip_control_t* mock_esp_sip_get_control(void) {
    return &mock_control;
}

void mock_esp_sip_reset(void) {
    memset(&mock_control, 0, sizeof(mock_control));
}

void mock_esp_sip_simulate_event(esp_sip_event_t event, void *event_data) {
    if (mock_control.registered_callback) {
        esp_sip_event_data_t sip_event = {
            .event = event
        };
        
        if (event_data) {
            memcpy(&sip_event.data, event_data, sizeof(sip_event.data));
        }
        
        mock_control.registered_callback(&sip_event, mock_control.callback_user_data);
    }
}

void mock_esp_sip_simulate_dtmf(char digit) {
    esp_sip_event_data_t event_data = {
        .event = ESP_SIP_EVENT_DTMF_RECEIVED,
        .data.dtmf.digit = digit
    };
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_DTMF_RECEIVED, &event_data.data);
}

// Mock implementations
esp_err_t esp_sip_init(esp_sip_config_t *config, esp_sip_event_callback_t callback, void *user_data, esp_sip_client_handle_t *client) {
    mock_control.init_call_count++;
    
    if (mock_control.init_should_fail) {
        return ESP_FAIL;
    }
    
    mock_control.registered_callback = callback;
    mock_control.callback_user_data = user_data;
    
    // Create a dummy client handle
    static int dummy_client = 1;
    *client = (esp_sip_client_handle_t)&dummy_client;
    mock_control.last_client = *client;
    
    return ESP_OK;
}

esp_err_t esp_sip_start(esp_sip_client_handle_t client) {
    mock_control.start_call_count++;
    
    if (mock_control.start_should_fail) {
        return ESP_FAIL;
    }
    
    // Simulate successful registration
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    return ESP_OK;
}

esp_err_t esp_sip_stop(esp_sip_client_handle_t client) {
    return ESP_OK;
}

esp_err_t esp_sip_call(esp_sip_client_handle_t client, const char *uri) {
    mock_control.call_call_count++;
    
    if (mock_control.call_should_fail) {
        return ESP_FAIL;
    }
    
    if (uri) {
        strncpy(mock_control.last_call_uri, uri, sizeof(mock_control.last_call_uri) - 1);
        mock_control.last_call_uri[sizeof(mock_control.last_call_uri) - 1] = '\0';
    }
    
    // Simulate call started
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_STARTED, NULL);
    
    return ESP_OK;
}

esp_err_t esp_sip_hangup(esp_sip_client_handle_t client) {
    mock_control.hangup_call_count++;
    
    if (mock_control.hangup_should_fail) {
        return ESP_FAIL;
    }
    
    // Simulate call ended
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
    
    return ESP_OK;
}

esp_err_t esp_sip_destroy(esp_sip_client_handle_t client) {
    mock_control.destroy_call_count++;
    return ESP_OK;
}