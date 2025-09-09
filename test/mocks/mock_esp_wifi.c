#include "mock_esp_wifi.h"
#include <string.h>
#include <stdlib.h>

// Mock state
static wifi_config_t last_config = {0};
static esp_err_t init_return = ESP_OK;
static esp_err_t config_return = ESP_OK;
static esp_err_t connect_return = ESP_OK;
static int connect_call_count = 0;
static bool mock_initialized = false;

void mock_esp_wifi_init(void) {
    memset(&last_config, 0, sizeof(wifi_config_t));
    init_return = ESP_OK;
    config_return = ESP_OK;
    connect_return = ESP_OK;
    connect_call_count = 0;
    mock_initialized = true;
}

void mock_esp_wifi_cleanup(void) {
    mock_initialized = false;
}

wifi_config_t* mock_esp_wifi_get_last_config(void) {
    if (!mock_initialized) {
        return NULL;
    }
    return &last_config;
}

void mock_esp_wifi_reset(void) {
    if (mock_initialized) {
        memset(&last_config, 0, sizeof(wifi_config_t));
        connect_call_count = 0;
    }
}

void mock_esp_wifi_set_init_return(esp_err_t ret_val) {
    init_return = ret_val;
}

void mock_esp_wifi_set_config_return(esp_err_t ret_val) {
    config_return = ret_val;
}

void mock_esp_wifi_set_connect_return(esp_err_t ret_val) {
    connect_return = ret_val;
}

int mock_esp_wifi_get_connect_call_count(void) {
    return connect_call_count;
}

// Mock implementations of ESP-IDF WiFi functions
esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    (void)config; // Unused parameter
    return init_return;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    (void)mode; // Unused parameter
    return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
    (void)interface; // Unused parameter
    
    if (mock_initialized && conf != NULL) {
        memcpy(&last_config, conf, sizeof(wifi_config_t));
    }
    
    return config_return;
}

esp_err_t esp_wifi_start(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_connect(void) {
    connect_call_count++;
    return connect_return;
}

esp_err_t esp_wifi_disconnect(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_stop(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_deinit(void) {
    return ESP_OK;
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) {
    if (ap_info != NULL) {
        memset(ap_info, 0, sizeof(wifi_ap_record_t));
        ap_info->rssi = -50; // Mock signal strength
        strcpy((char*)ap_info->ssid, "MockAP");
    }
    return ESP_OK;
}