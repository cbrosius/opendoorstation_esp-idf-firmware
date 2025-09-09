#include "mock_esp_event.h"
#include <string.h>

static bool mock_initialized = false;

void mock_esp_event_init(void) {
    mock_initialized = true;
}

void mock_esp_event_cleanup(void) {
    mock_initialized = false;
}

void mock_esp_event_reset(void) {
    // Nothing to reset for now
}

// Mock implementations of ESP-IDF event functions
esp_err_t esp_event_handler_register(esp_event_base_t event_base,
                                    int32_t event_id,
                                    esp_event_handler_t event_handler,
                                    void* event_handler_arg) {
    (void)event_base;
    (void)event_id;
    (void)event_handler;
    (void)event_handler_arg;
    
    return ESP_OK;
}

esp_err_t esp_event_handler_unregister(esp_event_base_t event_base,
                                      int32_t event_id,
                                      esp_event_handler_t event_handler) {
    (void)event_base;
    (void)event_id;
    (void)event_handler;
    
    return ESP_OK;
}