#include "mock_esp_netif.h"
#include <string.h>

static bool mock_initialized = false;

void mock_esp_netif_init(void) {
    mock_initialized = true;
}

void mock_esp_netif_cleanup(void) {
    mock_initialized = false;
}

void mock_esp_netif_reset(void) {
    // Nothing to reset for now
}

// Mock implementations of ESP-IDF netif functions
esp_err_t esp_netif_init(void) {
    return ESP_OK;
}

esp_netif_t* esp_netif_create_default_wifi_sta(void) {
    // Return a mock pointer (not NULL to indicate success)
    return (esp_netif_t*)0x12345678;
}

esp_err_t esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info) {
    (void)esp_netif; // Unused parameter
    
    if (ip_info != NULL) {
        // Mock IP configuration
        ip_info->ip.addr = 0xC0A80164; // 192.168.1.100
        ip_info->gw.addr = 0xC0A80101; // 192.168.1.1
        ip_info->netmask.addr = 0xFFFFFF00; // 255.255.255.0
    }
    
    return ESP_OK;
}

esp_netif_t* esp_netif_get_handle_from_ifkey(const char *if_key) {
    (void)if_key; // Unused parameter
    
    // Return a mock pointer
    return (esp_netif_t*)0x12345678;
}