#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "sip_door_station";

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 SIP Door Station starting...");
    
    // Basic initialization
    ESP_LOGI(TAG, "System initialized successfully");
    
    // Main application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}