#include "unity.h"
#include "web_server.h"
#include "io_manager.h"
#include "esp_log.h"

static const char *TAG = "test_web_ip_logging";

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
    if (web_server_is_running()) {
        web_server_stop();
    }
}

void test_web_server_ip_logging_when_running(void) {
    ESP_LOGI(TAG, "Testing web server IP logging when server is running");
    
    // Initialize I/O manager (required dependency)
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Test IP logging function
    result = web_server_log_url();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ESP_LOGI(TAG, "IP logging function executed successfully");
}

void test_web_server_ip_logging_when_not_running(void) {
    ESP_LOGI(TAG, "Testing web server IP logging when server is not running");
    
    // Ensure server is not running
    TEST_ASSERT_FALSE(web_server_is_running());
    
    // Try to log IP when server is not running
    esp_err_t result = web_server_log_url();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    ESP_LOGI(TAG, "IP logging correctly returns error when server not running");
}

void test_web_server_port_storage(void) {
    ESP_LOGI(TAG, "Testing web server port storage");
    
    // Initialize I/O manager
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server on specific port
    uint16_t test_port = 9090;
    result = web_server_init(test_port);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // The port should be stored internally and used for IP logging
    // We can't directly test the stored port, but we can verify the function works
    result = web_server_log_url();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ESP_LOGI(TAG, "Port storage and IP logging test completed");
}

void test_web_server_multiple_ip_log_calls(void) {
    ESP_LOGI(TAG, "Testing multiple IP logging calls");
    
    // Initialize I/O manager
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Call IP logging multiple times (should work each time)
    for (int i = 0; i < 3; i++) {
        result = web_server_log_url();
        TEST_ASSERT_EQUAL(ESP_OK, result);
    }
    
    ESP_LOGI(TAG, "Multiple IP logging calls completed successfully");
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_web_server_ip_logging_when_running);
    RUN_TEST(test_web_server_ip_logging_when_not_running);
    RUN_TEST(test_web_server_port_storage);
    RUN_TEST(test_web_server_multiple_ip_log_calls);
    
    UNITY_END();
}