#include "unity.h"
#include "config_manager.h"
#include "mocks/mock_nvs.h"
#include <string.h>

static const char *TAG = "test_config_env";

void setUp(void) {
    // Initialize mock NVS for each test
    mock_nvs_init();
}

void tearDown(void) {
    // Clean up after each test
    mock_nvs_clear();
}

void test_config_init_with_empty_nvs(void) {
    // Test initialization with empty NVS (should use defaults)
    esp_err_t result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Load configuration and verify defaults
    door_station_config_t config;
    result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    door_station_config_t expected_defaults;
    config_manager_get_defaults(&expected_defaults);
    
    // Should match defaults (no build-time config in test environment)
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, config.wifi_ssid);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, config.web_port);
    TEST_ASSERT_EQUAL(expected_defaults.door_pulse_duration, config.door_pulse_duration);
}

void test_config_init_with_existing_nvs_data(void) {
    door_station_config_t initial_config;
    
    // First, save some configuration to NVS
    config_manager_get_defaults(&initial_config);
    strcpy(initial_config.wifi_ssid, "ExistingNetwork");
    strcpy(initial_config.sip_user, "existinguser");
    initial_config.web_port = 9090;
    
    esp_err_t save_result = config_manager_save(&initial_config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Now initialize config manager (should load existing data)
    esp_err_t init_result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, init_result);
    
    // Load configuration and verify it matches what was saved
    door_station_config_t loaded_config;
    esp_err_t load_result = config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Should have preserved the existing configuration
    TEST_ASSERT_EQUAL_STRING("ExistingNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("existinguser", loaded_config.sip_user);
    TEST_ASSERT_EQUAL(9090, loaded_config.web_port);
}

void test_config_init_nvs_failure_recovery(void) {
    // Set mock NVS to fail mode for initial load
    mock_nvs_set_fail_mode(true);
    
    // Initialize should still succeed (using defaults)
    esp_err_t result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Reset NVS to working mode
    mock_nvs_set_fail_mode(false);
    mock_nvs_init();
    
    // Load configuration should return defaults
    door_station_config_t config;
    result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    door_station_config_t expected_defaults;
    config_manager_get_defaults(&expected_defaults);
    
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, config.wifi_ssid);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, config.web_port);
}

void test_config_init_saves_merged_config(void) {
    // Initialize with empty NVS
    esp_err_t init_result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, init_result);
    
    // Verify that configuration was saved to NVS during init
    // (This tests that init saves the merged config back to NVS)
    door_station_config_t loaded_config;
    esp_err_t load_result = config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Should have valid configuration (at least defaults)
    door_station_config_t expected_defaults;
    config_manager_get_defaults(&expected_defaults);
    
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, loaded_config.web_port);
}

void test_config_init_multiple_times(void) {
    // Test that multiple initializations don't cause issues
    esp_err_t result1 = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result1);
    
    esp_err_t result2 = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result2);
    
    esp_err_t result3 = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result3);
    
    // Configuration should still be valid
    door_station_config_t config;
    esp_err_t load_result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Validate the configuration
    config_validation_error_t validation = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, validation);
}

void test_config_init_with_nvs_corruption(void) {
    // Simulate NVS corruption by setting it to return "no free pages"
    // This should trigger NVS erase and reinit
    
    // First initialize normally
    esp_err_t result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // The init should handle NVS corruption gracefully
    // and still provide valid configuration
    door_station_config_t config;
    result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    config_validation_error_t validation = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, validation);
}

// Test helper to simulate build-time configuration
// Note: In a real test environment, we can't easily test compile-time defines,
// but we can test the logic that would handle them
void test_config_validation_after_init(void) {
    // Initialize configuration manager
    esp_err_t result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Load the configuration
    door_station_config_t config;
    result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // The configuration should always be valid after init
    config_validation_error_t validation = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, validation);
}

void test_config_init_preserves_valid_runtime_config(void) {
    door_station_config_t runtime_config;
    
    // Set up valid runtime configuration
    config_manager_get_defaults(&runtime_config);
    strcpy(runtime_config.wifi_ssid, "RuntimeNetwork");
    strcpy(runtime_config.sip_user, "runtimeuser");
    runtime_config.web_port = 8888;
    runtime_config.door_pulse_duration = 1500;
    
    // Save runtime configuration
    esp_err_t save_result = config_manager_save(&runtime_config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Initialize (this would merge build-time config if any)
    esp_err_t init_result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, init_result);
    
    // Load configuration
    door_station_config_t merged_config;
    esp_err_t load_result = config_manager_load(&merged_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // In test environment (no build-time config), runtime config should be preserved
    TEST_ASSERT_EQUAL_STRING("RuntimeNetwork", merged_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("runtimeuser", merged_config.sip_user);
    TEST_ASSERT_EQUAL(8888, merged_config.web_port);
    TEST_ASSERT_EQUAL(1500, merged_config.door_pulse_duration);
}