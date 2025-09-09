#include "unity.h"
#include "error_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "test_error_handler";

// Test callback data
static bool g_callback_called = false;
static error_info_t g_callback_error_info;

// Test callback function
static void test_error_callback(const error_info_t *error_info, void *user_data)
{
    g_callback_called = true;
    if (error_info) {
        memcpy(&g_callback_error_info, error_info, sizeof(error_info_t));
    }
}

void setUp(void)
{
    // Reset callback state
    g_callback_called = false;
    memset(&g_callback_error_info, 0, sizeof(error_info_t));
    
    // Initialize error handler for each test
    esp_err_t ret = error_handler_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void tearDown(void)
{
    // Clear error history after each test
    error_handler_clear_history();
}

void test_error_handler_init_success(void)
{
    ESP_LOGI(TAG, "Testing error handler initialization");
    
    // Should handle double initialization gracefully
    esp_err_t result = error_handler_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

void test_error_handler_report_basic(void)
{
    ESP_LOGI(TAG, "Testing basic error reporting");
    
    // Report a basic error
    uint32_t error_id = error_handler_report(ERROR_CATEGORY_SYSTEM, 
                                            ERROR_SEVERITY_ERROR,
                                            "test_component",
                                            ESP_ERR_INVALID_ARG,
                                            "Test error message");
    
    TEST_ASSERT_NOT_EQUAL(0, error_id);
    
    // Retrieve error information
    error_info_t error_info;
    esp_err_t result = error_handler_get_error_info(error_id, &error_info);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify error information
    TEST_ASSERT_EQUAL(error_id, error_info.error_id);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_SYSTEM, error_info.category);
    TEST_ASSERT_EQUAL(ERROR_SEVERITY_ERROR, error_info.severity);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, error_info.esp_error_code);
    TEST_ASSERT_EQUAL_STRING("test_component", error_info.component);
    TEST_ASSERT_EQUAL_STRING("Test error message", error_info.message);
    TEST_ASSERT_EQUAL(1, error_info.count);
    TEST_ASSERT_NOT_EQUAL(0, error_info.timestamp);
    TEST_ASSERT_EQUAL(error_info.timestamp, error_info.last_occurrence);
}

void test_error_handler_report_formatted(void)
{
    ESP_LOGI(TAG, "Testing formatted error reporting");
    
    // Report error with formatted message
    uint32_t error_id = error_handler_report(ERROR_CATEGORY_NETWORK, 
                                            ERROR_SEVERITY_WARNING,
                                            "wifi_manager",
                                            ESP_OK,
                                            "Connection failed after %d attempts, code: %d",
                                            3, 404);
    
    TEST_ASSERT_NOT_EQUAL(0, error_id);
    
    // Retrieve and verify
    error_info_t error_info;
    esp_err_t result = error_handler_get_error_info(error_id, &error_info);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    TEST_ASSERT_EQUAL_STRING("Connection failed after 3 attempts, code: 404", error_info.message);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_NETWORK, error_info.category);
    TEST_ASSERT_EQUAL(ERROR_SEVERITY_WARNING, error_info.severity);
}

void test_error_handler_duplicate_errors(void)
{
    ESP_LOGI(TAG, "Testing duplicate error handling");
    
    // Report same error multiple times
    uint32_t error_id1 = error_handler_report(ERROR_CATEGORY_SIP, 
                                             ERROR_SEVERITY_ERROR,
                                             "sip_manager",
                                             ESP_ERR_TIMEOUT,
                                             "Registration timeout");
    
    uint32_t error_id2 = error_handler_report(ERROR_CATEGORY_SIP, 
                                             ERROR_SEVERITY_ERROR,
                                             "sip_manager",
                                             ESP_ERR_TIMEOUT,
                                             "Registration timeout");
    
    // Should return same error ID for duplicates
    TEST_ASSERT_EQUAL(error_id1, error_id2);
    
    // Retrieve error information
    error_info_t error_info;
    esp_err_t result = error_handler_get_error_info(error_id1, &error_info);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Count should be 2
    TEST_ASSERT_EQUAL(2, error_info.count);
    TEST_ASSERT_GREATER_OR_EQUAL(error_info.timestamp, error_info.last_occurrence);
}

void test_error_handler_statistics(void)
{
    ESP_LOGI(TAG, "Testing error statistics");
    
    // Report various errors
    error_handler_report(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, "comp1", ESP_OK, "Error 1");
    error_handler_report(ERROR_CATEGORY_NETWORK, ERROR_SEVERITY_WARNING, "comp2", ESP_OK, "Error 2");
    error_handler_report(ERROR_CATEGORY_SIP, ERROR_SEVERITY_CRITICAL, "comp3", ESP_OK, "Error 3");
    error_handler_report(ERROR_CATEGORY_HARDWARE, ERROR_SEVERITY_INFO, "comp4", ESP_OK, "Error 4");
    
    // Get statistics
    error_stats_t stats;
    esp_err_t result = error_handler_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify statistics
    TEST_ASSERT_EQUAL(4, stats.total_errors);
    TEST_ASSERT_EQUAL(1, stats.errors_by_category[ERROR_CATEGORY_SYSTEM]);
    TEST_ASSERT_EQUAL(1, stats.errors_by_category[ERROR_CATEGORY_NETWORK]);
    TEST_ASSERT_EQUAL(1, stats.errors_by_category[ERROR_CATEGORY_SIP]);
    TEST_ASSERT_EQUAL(1, stats.errors_by_category[ERROR_CATEGORY_HARDWARE]);
    TEST_ASSERT_EQUAL(1, stats.critical_errors);
    TEST_ASSERT_NOT_EQUAL(0, stats.last_error_id);
    TEST_ASSERT_NOT_EQUAL(0, stats.uptime_at_last_error);
}

void test_error_handler_callback(void)
{
    ESP_LOGI(TAG, "Testing error callback");
    
    // Register callback
    esp_err_t result = error_handler_register_callback(test_error_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Report an error
    uint32_t error_id = error_handler_report(ERROR_CATEGORY_CONFIG, 
                                            ERROR_SEVERITY_ERROR,
                                            "config_manager",
                                            ESP_ERR_INVALID_STATE,
                                            "Invalid configuration");
    
    TEST_ASSERT_NOT_EQUAL(0, error_id);
    
    // Verify callback was called
    TEST_ASSERT_TRUE(g_callback_called);
    TEST_ASSERT_EQUAL(error_id, g_callback_error_info.error_id);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_CONFIG, g_callback_error_info.category);
    TEST_ASSERT_EQUAL(ERROR_SEVERITY_ERROR, g_callback_error_info.severity);
    TEST_ASSERT_EQUAL_STRING("config_manager", g_callback_error_info.component);
    TEST_ASSERT_EQUAL_STRING("Invalid configuration", g_callback_error_info.message);
}

void test_error_handler_clear_history(void)
{
    ESP_LOGI(TAG, "Testing error history clearing");
    
    // Report some errors
    error_handler_report(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_ERROR, "comp1", ESP_OK, "Error 1");
    error_handler_report(ERROR_CATEGORY_NETWORK, ERROR_SEVERITY_WARNING, "comp2", ESP_OK, "Error 2");
    
    // Verify errors exist
    error_stats_t stats;
    esp_err_t result = error_handler_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(2, stats.total_errors);
    
    // Clear history
    result = error_handler_clear_history();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify history is cleared
    result = error_handler_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(0, stats.total_errors);
    TEST_ASSERT_EQUAL(0, stats.critical_errors);
    TEST_ASSERT_EQUAL(0, stats.last_error_id);
}

void test_error_handler_utility_functions(void)
{
    ESP_LOGI(TAG, "Testing utility functions");
    
    // Test category strings
    TEST_ASSERT_EQUAL_STRING("SYSTEM", error_handler_get_category_string(ERROR_CATEGORY_SYSTEM));
    TEST_ASSERT_EQUAL_STRING("NETWORK", error_handler_get_category_string(ERROR_CATEGORY_NETWORK));
    TEST_ASSERT_EQUAL_STRING("SIP", error_handler_get_category_string(ERROR_CATEGORY_SIP));
    TEST_ASSERT_EQUAL_STRING("HARDWARE", error_handler_get_category_string(ERROR_CATEGORY_HARDWARE));
    TEST_ASSERT_EQUAL_STRING("CONFIG", error_handler_get_category_string(ERROR_CATEGORY_CONFIG));
    TEST_ASSERT_EQUAL_STRING("WEB", error_handler_get_category_string(ERROR_CATEGORY_WEB));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", error_handler_get_category_string((error_category_t)999));
    
    // Test severity strings
    TEST_ASSERT_EQUAL_STRING("INFO", error_handler_get_severity_string(ERROR_SEVERITY_INFO));
    TEST_ASSERT_EQUAL_STRING("WARNING", error_handler_get_severity_string(ERROR_SEVERITY_WARNING));
    TEST_ASSERT_EQUAL_STRING("ERROR", error_handler_get_severity_string(ERROR_SEVERITY_ERROR));
    TEST_ASSERT_EQUAL_STRING("CRITICAL", error_handler_get_severity_string(ERROR_SEVERITY_CRITICAL));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", error_handler_get_severity_string((error_severity_t)999));
    
    // Test recovery strings
    TEST_ASSERT_EQUAL_STRING("NONE", error_handler_get_recovery_string(ERROR_RECOVERY_NONE));
    TEST_ASSERT_EQUAL_STRING("RETRY", error_handler_get_recovery_string(ERROR_RECOVERY_RETRY));
    TEST_ASSERT_EQUAL_STRING("RESTART_SERVICE", error_handler_get_recovery_string(ERROR_RECOVERY_RESTART_SERVICE));
    TEST_ASSERT_EQUAL_STRING("FACTORY_RESET", error_handler_get_recovery_string(ERROR_RECOVERY_FACTORY_RESET));
    TEST_ASSERT_EQUAL_STRING("REBOOT", error_handler_get_recovery_string(ERROR_RECOVERY_REBOOT));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", error_handler_get_recovery_string((error_recovery_t)999));
}

void test_error_handler_critical_errors(void)
{
    ESP_LOGI(TAG, "Testing critical error detection");
    
    // Initially no critical errors
    bool has_critical = error_handler_has_critical_errors();
    TEST_ASSERT_FALSE(has_critical);
    
    // Report non-critical error
    error_handler_report(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_WARNING, "comp1", ESP_OK, "Warning");
    has_critical = error_handler_has_critical_errors();
    TEST_ASSERT_FALSE(has_critical);
    
    // Report critical error
    error_handler_report(ERROR_CATEGORY_SYSTEM, ERROR_SEVERITY_CRITICAL, "comp2", ESP_OK, "Critical error");
    has_critical = error_handler_has_critical_errors();
    TEST_ASSERT_TRUE(has_critical);
    
    // Clear and verify
    error_handler_clear_history();
    has_critical = error_handler_has_critical_errors();
    TEST_ASSERT_FALSE(has_critical);
}

void test_error_handler_recovery_actions(void)
{
    ESP_LOGI(TAG, "Testing recovery action configuration");
    
    // Set recovery action for a category
    esp_err_t result = error_handler_set_category_recovery(ERROR_CATEGORY_NETWORK, ERROR_RECOVERY_REBOOT);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Report error in that category
    uint32_t error_id = error_handler_report(ERROR_CATEGORY_NETWORK, 
                                            ERROR_SEVERITY_ERROR,
                                            "network_manager",
                                            ESP_OK,
                                            "Network failure");
    
    // Verify recovery action is set
    error_info_t error_info;
    result = error_handler_get_error_info(error_id, &error_info);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(ERROR_RECOVERY_REBOOT, error_info.recovery);
    
    // Test invalid category
    result = error_handler_set_category_recovery((error_category_t)999, ERROR_RECOVERY_NONE);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_error_handler_invalid_parameters(void)
{
    ESP_LOGI(TAG, "Testing invalid parameter handling");
    
    // Test invalid parameters for error reporting
    uint32_t error_id = error_handler_report((error_category_t)999, 
                                            ERROR_SEVERITY_ERROR,
                                            "test_component",
                                            ESP_OK,
                                            "Test message");
    TEST_ASSERT_EQUAL(0, error_id);
    
    error_id = error_handler_report(ERROR_CATEGORY_SYSTEM, 
                                   ERROR_SEVERITY_ERROR,
                                   NULL,  // Invalid component
                                   ESP_OK,
                                   "Test message");
    TEST_ASSERT_EQUAL(0, error_id);
    
    error_id = error_handler_report(ERROR_CATEGORY_SYSTEM, 
                                   ERROR_SEVERITY_ERROR,
                                   "test_component",
                                   ESP_OK,
                                   NULL);  // Invalid format
    TEST_ASSERT_EQUAL(0, error_id);
    
    // Test invalid parameters for get_error_info
    error_info_t error_info;
    esp_err_t result = error_handler_get_error_info(999999, &error_info);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, result);
    
    result = error_handler_get_error_info(1, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
    
    // Test invalid parameters for get_stats
    result = error_handler_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_error_handler_convenience_macros(void)
{
    ESP_LOGI(TAG, "Testing convenience macros");
    
    // Test all convenience macros
    uint32_t id1 = ERROR_REPORT_SYSTEM(ERROR_SEVERITY_ERROR, "test", ESP_OK, "System error");
    uint32_t id2 = ERROR_REPORT_NETWORK(ERROR_SEVERITY_WARNING, "test", ESP_OK, "Network error");
    uint32_t id3 = ERROR_REPORT_SIP(ERROR_SEVERITY_ERROR, "test", ESP_OK, "SIP error");
    uint32_t id4 = ERROR_REPORT_HARDWARE(ERROR_SEVERITY_CRITICAL, "test", ESP_OK, "Hardware error");
    uint32_t id5 = ERROR_REPORT_CONFIG(ERROR_SEVERITY_WARNING, "test", ESP_OK, "Config error");
    uint32_t id6 = ERROR_REPORT_WEB(ERROR_SEVERITY_ERROR, "test", ESP_OK, "Web error");
    
    TEST_ASSERT_NOT_EQUAL(0, id1);
    TEST_ASSERT_NOT_EQUAL(0, id2);
    TEST_ASSERT_NOT_EQUAL(0, id3);
    TEST_ASSERT_NOT_EQUAL(0, id4);
    TEST_ASSERT_NOT_EQUAL(0, id5);
    TEST_ASSERT_NOT_EQUAL(0, id6);
    
    // Verify categories are correct
    error_info_t error_info;
    error_handler_get_error_info(id1, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_SYSTEM, error_info.category);
    
    error_handler_get_error_info(id2, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_NETWORK, error_info.category);
    
    error_handler_get_error_info(id3, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_SIP, error_info.category);
    
    error_handler_get_error_info(id4, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_HARDWARE, error_info.category);
    
    error_handler_get_error_info(id5, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_CONFIG, error_info.category);
    
    error_handler_get_error_info(id6, &error_info);
    TEST_ASSERT_EQUAL(ERROR_CATEGORY_WEB, error_info.category);
}

// Test runner
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_error_handler_init_success);
    RUN_TEST(test_error_handler_report_basic);
    RUN_TEST(test_error_handler_report_formatted);
    RUN_TEST(test_error_handler_duplicate_errors);
    RUN_TEST(test_error_handler_statistics);
    RUN_TEST(test_error_handler_callback);
    RUN_TEST(test_error_handler_clear_history);
    RUN_TEST(test_error_handler_utility_functions);
    RUN_TEST(test_error_handler_critical_errors);
    RUN_TEST(test_error_handler_recovery_actions);
    RUN_TEST(test_error_handler_invalid_parameters);
    RUN_TEST(test_error_handler_convenience_macros);
    
    UNITY_END();
}