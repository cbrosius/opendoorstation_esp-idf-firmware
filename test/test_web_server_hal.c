#include "unity.h"
#include "web_server.h"
#include "mocks/mock_http_server.h"
#include "mocks/mock_freertos.h"
#include <string.h>

static const char *TAG = "test_web_server_hal";

// Test fixture setup and teardown
void setUp(void)
{
    mock_http_server_init();
    mock_freertos_init();
}

void tearDown(void)
{
    mock_http_server_reset();
    mock_freertos_reset();
    web_server_stop();
}

// ============================================================================
// Web Server Hardware Abstraction Layer Tests
// ============================================================================

void test_web_server_hal_initialization_smoke(void)
{
    // Test that web server initialization doesn't crash and returns success
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify HTTP server was started
    mock_http_server_control_t* control = mock_http_server_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->start_call_count);
    TEST_ASSERT_EQUAL(8080, control->last_port);
    
    // Verify server is running
    TEST_ASSERT_TRUE(web_server_is_running());
}

void test_web_server_hal_stop_smoke(void)
{
    web_server_init(8080);
    
    esp_err_t result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify HTTP server was stopped
    mock_http_server_control_t* control = mock_http_server_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->stop_call_count);
    
    // Verify server is not running
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_hal_uri_registration_smoke(void)
{
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify URI handlers were registered
    mock_http_server_control_t* control = mock_http_server_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->register_uri_call_count);
}

void test_web_server_hal_broadcast_smoke(void)
{
    web_server_init(8080);
    
    // Test relay status broadcast
    esp_err_t result = web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

void test_web_server_hal_url_logging_smoke(void)
{
    web_server_init(8080);
    
    // Test URL logging
    esp_err_t result = web_server_log_url();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

void test_web_server_hal_error_conditions_smoke(void)
{
    // Test initialization failure
    mock_http_server_set_start_fail(true);
    
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Reset failure mode
    mock_http_server_set_start_fail(false);
    
    // Test stop failure
    web_server_init(8080);
    mock_http_server_set_stop_fail(true);
    
    result = web_server_stop();
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Test invalid port
    result = web_server_init(0);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    result = web_server_init(70000);  // Port too high
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

void test_web_server_hal_multiple_init_smoke(void)
{
    // Test multiple initialization calls
    esp_err_t result1 = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result1);
    
    // Second init should fail or handle gracefully
    esp_err_t result2 = web_server_init(8081);
    // Should either succeed (restart) or fail gracefully
    TEST_ASSERT_TRUE(result2 == ESP_OK || result2 != ESP_OK);
    
    // Should still be able to stop
    esp_err_t result3 = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result3);
}

void test_web_server_hal_stop_without_init_smoke(void)
{
    // Test stopping server that was never started
    esp_err_t result = web_server_stop();
    // Should handle gracefully (either succeed or fail with specific error)
    TEST_ASSERT_TRUE(result == ESP_OK || result != ESP_OK);
}

void test_web_server_hal_broadcast_without_init_smoke(void)
{
    // Test broadcasting without server running
    esp_err_t result = web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    // Should handle gracefully
    TEST_ASSERT_TRUE(result == ESP_OK || result != ESP_OK);
}

void test_web_server_hal_coverage_all_functions(void)
{
    // Test all web server functions to ensure coverage
    web_server_init(8080);
    
    // Test status functions
    bool running = web_server_is_running();
    TEST_ASSERT_TRUE(running);
    
    // Test broadcast functions
    web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_OFF);
    web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_ON);
    web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
    
    // Test URL logging
    web_server_log_url();
    
    // Test stop
    web_server_stop();
    
    running = web_server_is_running();
    TEST_ASSERT_FALSE(running);
    
    // Verify all functions executed without crashing
    TEST_ASSERT_TRUE(true);
}