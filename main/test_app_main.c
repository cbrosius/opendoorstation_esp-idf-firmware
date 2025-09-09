#include <stdio.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "test_app_main";

// External test function declarations - these come from test_main.c
extern void app_main_tests(void);

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 SIP Door Station - Unity Test Framework");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "Starting comprehensive test suite...");
    ESP_LOGI(TAG, "");
    
    // Give the system a moment to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Run all tests
    app_main_tests();
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Test execution completed.");
    ESP_LOGI(TAG, "Check the output above for test results.");
    
    // Keep the system running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "System running... Press reset to run tests again.");
    }
}