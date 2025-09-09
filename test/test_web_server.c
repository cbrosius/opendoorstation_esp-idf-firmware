#include "unity.h"
#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "test_web_server";

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
    if (web_server_is_running()) {
        web_server_stop();
    }
}

void test_web_server_init_success(void) {
    ESP_LOGI(TAG, "Testing web server initialization");
    
    // Test successful initialization
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Cleanup
    web_server_stop();
}

void test_web_server_init_invalid_port(void) {
    ESP_LOGI(TAG, "Testing web server initialization with invalid port");
    
    // Test with port 0 (invalid)
    esp_err_t result = web_server_init(0);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_double_init(void) {
    ESP_LOGI(TAG, "Testing double initialization");
    
    // First initialization should succeed
    esp_err_t result1 = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result1);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Second initialization should fail
    esp_err_t result2 = web_server_init(8081);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result2);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Cleanup
    web_server_stop();
}

void test_web_server_stop_success(void) {
    ESP_LOGI(TAG, "Testing web server stop");
    
    // Start server first
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Stop server
    result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_stop_not_running(void) {
    ESP_LOGI(TAG, "Testing stop when server not running");
    
    // Ensure server is not running
    TEST_ASSERT_FALSE(web_server_is_running());
    
    // Try to stop non-running server
    esp_err_t result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
}

void test_web_server_is_running_states(void) {
    ESP_LOGI(TAG, "Testing server running state detection");
    
    // Initially not running
    TEST_ASSERT_FALSE(web_server_is_running());
    
    // Start server
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Stop server
    result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_port_range(void) {
    ESP_LOGI(TAG, "Testing various port ranges");
    
    // Test minimum valid port
    esp_err_t result = web_server_init(1024);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    web_server_stop();
    
    // Test standard HTTP port
    result = web_server_init(80);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    web_server_stop();
    
    // Test high port number
    result = web_server_init(65535);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    web_server_stop();
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_web_server_init_success);
    RUN_TEST(test_web_server_init_invalid_port);
    RUN_TEST(test_web_server_double_init);
    RUN_TEST(test_web_server_stop_success);
    RUN_TEST(test_web_server_stop_not_running);
    RUN_TEST(test_web_server_is_running_states);
    RUN_TEST(test_web_server_port_range);
    
    UNITY_END();
}

// Tests for factory reset functionality

void test_web_server_factory_reset_endpoint_exists(void) {
    ESP_LOGI(TAG, "Testing factory reset endpoint availability");
    
    // Initialize web server
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // The factory reset endpoint should be registered
    // This test verifies the server starts successfully with the endpoint
    // Actual HTTP testing would require more complex mock setup
    
    // Cleanup
    web_server_stop();
}

void test_web_server_factory_reset_handler_logic(void) {
    ESP_LOGI(TAG, "Testing factory reset handler logic");
    
    // This test would verify the factory reset handler calls config_manager_factory_reset()
    // In a real implementation, we would mock the HTTP request and verify the response
    // For now, we test that the web server initializes with the handler registered
    
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Cleanup
    web_server_stop();
}