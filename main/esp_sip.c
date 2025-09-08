#include "esp_sip.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "esp_sip";

struct esp_sip_client {
    esp_sip_config_t config;
    esp_sip_event_callback_t callback;
    void *user_data;
    bool started;
};

esp_err_t esp_sip_init(esp_sip_config_t *config, esp_sip_event_callback_t callback, void *user_data, esp_sip_client_handle_t *client) {
    if (!config || !callback || !client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    struct esp_sip_client *sip_client = malloc(sizeof(struct esp_sip_client));
    if (!sip_client) {
        return ESP_ERR_NO_MEM;
    }
    
    memcpy(&sip_client->config, config, sizeof(esp_sip_config_t));
    sip_client->callback = callback;
    sip_client->user_data = user_data;
    sip_client->started = false;
    
    *client = sip_client;
    
    ESP_LOGI(TAG, "SIP client initialized");
    return ESP_OK;
}

esp_err_t esp_sip_start(esp_sip_client_handle_t client) {
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    client->started = true;
    ESP_LOGI(TAG, "SIP client started");
    
    // Simulate registration success
    esp_sip_event_data_t event = {
        .event = ESP_SIP_EVENT_REGISTERED
    };
    client->callback(&event, client->user_data);
    
    return ESP_OK;
}

esp_err_t esp_sip_stop(esp_sip_client_handle_t client) {
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    client->started = false;
    ESP_LOGI(TAG, "SIP client stopped");
    return ESP_OK;
}

esp_err_t esp_sip_call(esp_sip_client_handle_t client, const char *uri) {
    if (!client || !uri) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Making call to: %s", uri);
    
    // Simulate call started
    esp_sip_event_data_t event = {
        .event = ESP_SIP_EVENT_CALL_STARTED
    };
    client->callback(&event, client->user_data);
    
    return ESP_OK;
}

esp_err_t esp_sip_hangup(esp_sip_client_handle_t client) {
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Hanging up call");
    
    // Simulate call ended
    esp_sip_event_data_t event = {
        .event = ESP_SIP_EVENT_CALL_ENDED
    };
    client->callback(&event, client->user_data);
    
    return ESP_OK;
}

esp_err_t esp_sip_destroy(esp_sip_client_handle_t client) {
    if (!client) {
        return ESP_ERR_INVALID_ARG;
    }
    
    free(client);
    ESP_LOGI(TAG, "SIP client destroyed");
    return ESP_OK;
}