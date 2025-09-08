#include "io_events.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "io_events";

// Define the event base
ESP_EVENT_DEFINE_BASE(IO_EVENTS);

esp_err_t io_events_init(void)
{
    ESP_LOGI(TAG, "Initializing I/O event system");
    
    // The default event loop should already be created by the main application
    // We just need to ensure it's available for our events
    
    ESP_LOGI(TAG, "I/O event system initialized");
    return ESP_OK;
}

esp_err_t io_events_publish_button(bool pressed)
{
    io_button_event_data_t event_data = {
        .pressed = pressed,
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000) // Convert to milliseconds
    };
    
    io_event_id_t event_id = pressed ? IO_EVENT_BUTTON_PRESSED : IO_EVENT_BUTTON_RELEASED;
    
    esp_err_t ret = esp_event_post(IO_EVENTS, event_id, &event_data, sizeof(event_data), 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to publish button event: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGD(TAG, "Published button %s event", pressed ? "pressed" : "released");
    return ESP_OK;
}

esp_err_t io_events_publish_relay_state_change(relay_id_t relay, 
                                               relay_state_t old_state, 
                                               relay_state_t new_state)
{
    io_relay_event_data_t event_data = {
        .relay = relay,
        .old_state = old_state,
        .new_state = new_state,
        .timestamp = (uint32_t)(esp_timer_get_time() / 1000) // Convert to milliseconds
    };
    
    esp_err_t ret = esp_event_post(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                                   &event_data, sizeof(event_data), 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to publish relay state change event: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGD(TAG, "Published relay %d state change: %s -> %s", 
             relay,
             old_state == RELAY_STATE_ON ? "ON" : "OFF",
             new_state == RELAY_STATE_ON ? "ON" : "OFF");
    
    return ESP_OK;
}