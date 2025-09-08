#include "unity.h"
#include "config_manager.h"
#include "mocks/mock_nvs.h"
#include <string.h>

static const char *TAG = "test_config_storage";

void setUp(void) {
    // Initialize mock NVS for each test
    mock_nvs_init();
    config_manager_init();
}

void tearDown(void) {
    // Clean up after each test
    mock_nvs_clear();
}

void test_config_save_and_load_success(void) {
    door_station_config_t save_config, load_config;
    
    // Prepare test configuration
    config_manager_get_defaults(&save_config);
    strcpy(save_config.wifi_ssid, "TestNetwork");
    strcpy(save_config.wifi_password, "password123");
    strcpy(save_config.sip_user, "testuser");
    strcpy(save_config.sip_domain, "192.168.1.100");
    strcpy(save_config.sip_password, "sippass");
    strcpy(save_config.sip_callee, "sip:doorbell@192.168.1.100");
    save_config.web_port = 9090;
    save_config.door_pulse_duration = 3000;
    
    // Save configuration
    esp_err_t save_result = config_manager_save(&save_config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Verify entries were created in mock NVS
    TEST_ASSERT_TRUE(mock_nvs_key_exists("wifi_ssid"));
    TEST_ASSERT_TRUE(mock_nvs_key_exists("sip_user"));
    TEST_ASSERT_TRUE(mock_nvs_key_exists("web_port"));
    TEST_ASSERT_TRUE(mock_nvs_key_exists("door_pulse_duration"));
    
    // Load configuration
    esp_err_t load_result = config_manager_load(&load_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Verify loaded values match saved values
    TEST_ASSERT_EQUAL_STRING(save_config.wifi_ssid, load_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(save_config.wifi_password, load_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING(save_config.sip_user, load_config.sip_user);
    TEST_ASSERT_EQUAL_STRING(save_config.sip_domain, load_config.sip_domain);
    TEST_ASSERT_EQUAL_STRING(save_config.sip_password, load_config.sip_password);
    TEST_ASSERT_EQUAL_STRING(save_config.sip_callee, load_config.sip_callee);
    TEST_ASSERT_EQUAL(save_config.web_port, load_config.web_port);
    TEST_ASSERT_EQUAL(save_config.door_pulse_duration, load_config.door_pulse_duration);
}

void test_config_load_defaults_when_nvs_empty(void) {
    door_station_config_t config, expected_defaults;
    
    // Get expected defaults
    config_manager_get_defaults(&expected_defaults);
    
    // Load configuration from empty NVS
    esp_err_t result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify defaults are loaded
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_password, config.wifi_password);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.sip_user, config.sip_user);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.sip_domain, config.sip_domain);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.sip_password, config.sip_password);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.sip_callee, config.sip_callee);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, config.web_port);
    TEST_ASSERT_EQUAL(expected_defaults.door_pulse_duration, config.door_pulse_duration);
}

void test_config_save_invalid_config_fails(void) {
    door_station_config_t config;
    
    // Create invalid configuration (port out of range)
    config_manager_get_defaults(&config);
    config.web_port = 80;  // Invalid port (too low)
    
    // Attempt to save invalid configuration
    esp_err_t result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
    
    // Verify no entries were created in NVS
    TEST_ASSERT_EQUAL(0, mock_nvs_get_entry_count());
}

void test_config_save_nvs_failure(void) {
    door_station_config_t config;
    
    // Prepare valid configuration
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    
    // Set mock NVS to fail mode
    mock_nvs_set_fail_mode(true);
    
    // Attempt to save configuration
    esp_err_t result = config_manager_save(&config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

void test_config_load_nvs_failure(void) {
    door_station_config_t config;
    
    // Set mock NVS to fail mode
    mock_nvs_set_fail_mode(true);
    
    // Attempt to load configuration (should return defaults)
    esp_err_t result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Should have loaded defaults despite NVS failure
    door_station_config_t expected_defaults;
    config_manager_get_defaults(&expected_defaults);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, config.wifi_ssid);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, config.web_port);
}

void test_config_factory_reset(void) {
    door_station_config_t config;
    
    // First, save some configuration
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "testuser");
    config.web_port = 9090;
    
    esp_err_t save_result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Verify configuration was saved
    TEST_ASSERT_TRUE(mock_nvs_key_exists("wifi_ssid"));
    TEST_ASSERT_TRUE(mock_nvs_key_exists("sip_user"));
    
    // Perform factory reset
    esp_err_t reset_result = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset_result);
    
    // Load configuration after reset
    door_station_config_t reset_config;
    esp_err_t load_result = config_manager_load(&reset_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Verify defaults are loaded (configuration was cleared)
    door_station_config_t expected_defaults;
    config_manager_get_defaults(&expected_defaults);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.wifi_ssid, reset_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(expected_defaults.sip_user, reset_config.sip_user);
    TEST_ASSERT_EQUAL(expected_defaults.web_port, reset_config.web_port);
}

void test_config_factory_reset_nvs_failure(void) {
    // Set mock NVS to fail mode
    mock_nvs_set_fail_mode(true);
    
    // Attempt factory reset
    esp_err_t result = config_manager_factory_reset();
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

void test_config_save_load_null_pointer(void) {
    // Test save with NULL pointer
    esp_err_t save_result = config_manager_save(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, save_result);
    
    // Test load with NULL pointer
    esp_err_t load_result = config_manager_load(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, load_result);
}

void test_config_partial_load_with_missing_keys(void) {
    door_station_config_t config;
    
    // Manually add only some keys to mock NVS
    mock_nvs_init();
    
    // Simulate having only some configuration keys stored
    nvs_handle_t handle;
    nvs_open("door_station", NVS_READWRITE, &handle);
    nvs_set_str(handle, "wifi_ssid", "PartialNetwork");
    nvs_set_u16(handle, "web_port", 9999);
    nvs_close(handle);
    
    // Load configuration
    esp_err_t result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify partial values were loaded and others are defaults
    TEST_ASSERT_EQUAL_STRING("PartialNetwork", config.wifi_ssid);
    TEST_ASSERT_EQUAL(9999, config.web_port);
    
    // These should be defaults since they weren't stored
    door_station_config_t defaults;
    config_manager_get_defaults(&defaults);
    TEST_ASSERT_EQUAL_STRING(defaults.sip_user, config.sip_user);
    TEST_ASSERT_EQUAL(defaults.door_pulse_duration, config.door_pulse_duration);
}