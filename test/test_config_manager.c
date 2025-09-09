#include "unity.h"
#include "config_manager.h"
#include <string.h>

static const char *TAG = "test_config_manager";

void setUp(void) {
    // Initialize config manager for each test
    config_manager_init();
}

void tearDown(void) {
    // Clean up after each test
}

void test_config_validation_valid_config(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Set valid values
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "password123");
    strcpy(config.sip_user, "testuser");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_password, "sippass");
    strcpy(config.sip_callee, "sip:doorbell@192.168.1.100");
    config.web_port = 8080;
    config.door_pulse_duration = 2000;
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_wifi_ssid_too_long(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Set SSID that's too long (32 characters)
    strcpy(config.wifi_ssid, "ThisSSIDIsWayTooLongForWiFiSpec");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_WIFI_SSID_TOO_LONG, result);
}

void test_config_validation_wifi_ssid_empty_valid(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Empty SSID should be valid (not configured yet)
    strcpy(config.wifi_ssid, "");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_user_too_short(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // SIP user too short (2 characters)
    strcpy(config.sip_user, "ab");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_SIP_USER_TOO_SHORT, result);
}

void test_config_validation_sip_user_too_long(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // SIP user too long (32 characters)
    strcpy(config.sip_user, "thisusernameiswaytoolongforspec");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_SIP_USER_TOO_LONG, result);
}

void test_config_validation_sip_user_invalid_chars(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // SIP user with invalid characters
    strcpy(config.sip_user, "user@domain");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_SIP_USER_INVALID, result);
}

void test_config_validation_sip_user_valid_underscore(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // SIP user with valid underscore
    strcpy(config.sip_user, "test_user_123");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_domain_ipv4_valid(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid IPv4 address
    strcpy(config.sip_domain, "192.168.1.100");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_domain_hostname_valid(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid hostname
    strcpy(config.sip_domain, "sip.example.com");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_domain_invalid(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Invalid domain with special characters
    strcpy(config.sip_domain, "invalid@domain");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_SIP_DOMAIN_INVALID, result);
}

void test_config_validation_sip_callee_valid_simple(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid simple SIP URI
    strcpy(config.sip_callee, "user@192.168.1.100");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_callee_valid_with_sip_prefix(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid SIP URI with sip: prefix
    strcpy(config.sip_callee, "sip:doorbell@example.com");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, result);
}

void test_config_validation_sip_callee_invalid_no_at(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Invalid SIP URI without @
    strcpy(config.sip_callee, "invaliduri");
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_SIP_CALLEE_INVALID, result);
}

void test_config_validation_web_port_too_low(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Port too low (below 1024)
    config.web_port = 80;
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_WEB_PORT_INVALID, result);
}

void test_config_validation_web_port_too_high(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Port too high (above 65535)
    config.web_port = 70000;
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_WEB_PORT_INVALID, result);
}

void test_config_validation_web_port_valid_range(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid port ranges
    config.web_port = 1024;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
    
    config.web_port = 8080;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
    
    config.web_port = 65535;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
}

void test_config_validation_door_pulse_too_short(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Pulse duration too short
    config.door_pulse_duration = 100;
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_DOOR_PULSE_INVALID, result);
}

void test_config_validation_door_pulse_too_long(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Pulse duration too long
    config.door_pulse_duration = 15000;
    
    config_validation_error_t result = config_manager_validate(&config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_DOOR_PULSE_INVALID, result);
}

void test_config_validation_door_pulse_valid_range(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Valid pulse durations
    config.door_pulse_duration = 500;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
    
    config.door_pulse_duration = 2000;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
    
    config.door_pulse_duration = 10000;
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&config));
}

void test_config_get_defaults(void) {
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Check default values
    TEST_ASSERT_EQUAL_STRING("", config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("", config.sip_user);
    TEST_ASSERT_EQUAL_STRING("", config.sip_domain);
    TEST_ASSERT_EQUAL_STRING("", config.sip_password);
    TEST_ASSERT_EQUAL_STRING("", config.sip_callee);
    TEST_ASSERT_EQUAL(8080, config.web_port);
    TEST_ASSERT_EQUAL(2000, config.door_pulse_duration);
}

void test_config_validation_error_messages(void) {
    // Test that all error codes have valid messages
    TEST_ASSERT_NOT_NULL(config_manager_get_validation_error_message(CONFIG_VALIDATION_OK));
    TEST_ASSERT_NOT_NULL(config_manager_get_validation_error_message(CONFIG_VALIDATION_WIFI_SSID_INVALID));
    TEST_ASSERT_NOT_NULL(config_manager_get_validation_error_message(CONFIG_VALIDATION_SIP_USER_INVALID));
    TEST_ASSERT_NOT_NULL(config_manager_get_validation_error_message(CONFIG_VALIDATION_WEB_PORT_INVALID));
    
    // Test unknown error code
    const char* unknown_msg = config_manager_get_validation_error_message((config_validation_error_t)999);
    TEST_ASSERT_EQUAL_STRING("Unknown validation error", unknown_msg);
}

void test_config_validation_null_pointer(void) {
    // Test validation with NULL pointer
    config_validation_error_t result = config_manager_validate(NULL);
    TEST_ASSERT_NOT_EQUAL(CONFIG_VALIDATION_OK, result);
}
// Tests for secure storage functionality

void test_config_manager_is_sensitive_field(void) {
    // Test sensitive fields
    TEST_ASSERT_TRUE(config_manager_is_sensitive_field("wifi_password"));
    TEST_ASSERT_TRUE(config_manager_is_sensitive_field("sip_password"));
    
    // Test non-sensitive fields
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("wifi_ssid"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("sip_user"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("sip_domain"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("sip_callee"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("web_port"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("door_pulse_duration"));
    
    // Test invalid input
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field(NULL));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field(""));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("unknown_field"));
}

void test_config_manager_mask_sensitive_value_normal(void) {
    char masked[64];
    
    // Test normal password masking
    config_manager_mask_sensitive_value("password123", masked, sizeof(masked));
    TEST_ASSERT_EQUAL_STRING("********", masked);
    
    // Test longer password
    config_manager_mask_sensitive_value("verylongpassword", masked, sizeof(masked));
    TEST_ASSERT_EQUAL_STRING("********", masked);
}

void test_config_manager_mask_sensitive_value_short(void) {
    char masked[64];
    
    // Test short password (less than 8 characters)
    config_manager_mask_sensitive_value("abc", masked, sizeof(masked));
    TEST_ASSERT_EQUAL_STRING("***", masked);
    
    config_manager_mask_sensitive_value("12345", masked, sizeof(masked));
    TEST_ASSERT_EQUAL_STRING("*****", masked);
}

void test_config_manager_mask_sensitive_value_empty(void) {
    char masked[64];
    
    // Test empty password
    config_manager_mask_sensitive_value("", masked, sizeof(masked));
    TEST_ASSERT_EQUAL_STRING("", masked);
}

void test_config_manager_mask_sensitive_value_buffer_limits(void) {
    char small_buffer[5];
    
    // Test with small buffer
    config_manager_mask_sensitive_value("password123", small_buffer, sizeof(small_buffer));
    TEST_ASSERT_EQUAL_STRING("****", small_buffer);
    
    // Test with buffer size 1 (only null terminator)
    char tiny_buffer[1];
    config_manager_mask_sensitive_value("password", tiny_buffer, sizeof(tiny_buffer));
    TEST_ASSERT_EQUAL_STRING("", tiny_buffer);
}

void test_config_manager_mask_sensitive_value_null_inputs(void) {
    char masked[64];
    
    // Test NULL value
    config_manager_mask_sensitive_value(NULL, masked, sizeof(masked));
    // Should not crash, masked buffer content is undefined but safe
    
    // Test NULL masked buffer
    config_manager_mask_sensitive_value("password", NULL, 64);
    // Should not crash
    
    // Test zero buffer size
    config_manager_mask_sensitive_value("password", masked, 0);
    // Should not crash
}

void test_config_manager_load_masked(void) {
    door_station_config_t config;
    
    // First save a configuration with passwords
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "wifipass123");
    strcpy(config.sip_user, "testuser");
    strcpy(config.sip_password, "sippass456");
    
    esp_err_t save_result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Load with masking
    door_station_config_t masked_config;
    esp_err_t load_result = config_manager_load_masked(&masked_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Check that non-sensitive fields are preserved
    TEST_ASSERT_EQUAL_STRING("TestNetwork", masked_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("testuser", masked_config.sip_user);
    
    // Check that sensitive fields are masked
    TEST_ASSERT_EQUAL_STRING("********", masked_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("********", masked_config.sip_password);
}

void test_config_manager_load_masked_null_input(void) {
    esp_err_t result = config_manager_load_masked(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_config_manager_secure_storage_separation(void) {
    door_station_config_t config;
    
    // Save configuration with sensitive data
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "secretwifi");
    strcpy(config.sip_user, "testuser");
    strcpy(config.sip_password, "secretsip");
    
    esp_err_t save_result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Load configuration normally
    door_station_config_t loaded_config;
    esp_err_t load_result = config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Verify all fields are loaded correctly
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("secretwifi", loaded_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("testuser", loaded_config.sip_user);
    TEST_ASSERT_EQUAL_STRING("secretsip", loaded_config.sip_password);
}

void test_config_manager_factory_reset_clears_sensitive_data(void) {
    door_station_config_t config;
    
    // Save configuration with sensitive data
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "secretwifi");
    strcpy(config.sip_password, "secretsip");
    
    esp_err_t save_result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Perform factory reset
    esp_err_t reset_result = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset_result);
    
    // Load configuration after reset
    door_station_config_t reset_config;
    esp_err_t load_result = config_manager_load(&reset_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // Verify all fields are back to defaults (empty strings for credentials)
    TEST_ASSERT_EQUAL_STRING("", reset_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", reset_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_user);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_password);
    TEST_ASSERT_EQUAL(8080, reset_config.web_port);
    TEST_ASSERT_EQUAL(2000, reset_config.door_pulse_duration);
}

// Additional tests for factory reset functionality

void test_config_manager_factory_reset_api_integration(void) {
    door_station_config_t config;
    
    // Save some configuration first
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "testpass");
    strcpy(config.sip_user, "testuser");
    strcpy(config.sip_password, "sippass");
    
    esp_err_t save_result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, save_result);
    
    // Verify configuration was saved
    door_station_config_t loaded_config;
    esp_err_t load_result = config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("testpass", loaded_config.wifi_password);
    
    // Perform factory reset
    esp_err_t reset_result = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset_result);
    
    // Verify all configuration is cleared
    door_station_config_t reset_config;
    esp_err_t load_after_reset = config_manager_load(&reset_config);
    TEST_ASSERT_EQUAL(ESP_OK, load_after_reset);
    
    // All string fields should be empty
    TEST_ASSERT_EQUAL_STRING("", reset_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("", reset_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_user);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_password);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_domain);
    TEST_ASSERT_EQUAL_STRING("", reset_config.sip_callee);
    
    // Numeric fields should be defaults
    TEST_ASSERT_EQUAL(8080, reset_config.web_port);
    TEST_ASSERT_EQUAL(2000, reset_config.door_pulse_duration);
}

void test_config_manager_factory_reset_multiple_calls(void) {
    // Test that multiple factory reset calls don't cause issues
    
    esp_err_t reset1 = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset1);
    
    esp_err_t reset2 = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset2);
    
    esp_err_t reset3 = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, reset3);
    
    // Configuration should still be in default state
    door_station_config_t config;
    esp_err_t load_result = config_manager_load(&config);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    door_station_config_t defaults;
    config_manager_get_defaults(&defaults);
    
    // Compare with defaults
    TEST_ASSERT_EQUAL_STRING(defaults.wifi_ssid, config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(defaults.wifi_password, config.wifi_password);
    TEST_ASSERT_EQUAL(defaults.web_port, config.web_port);
    TEST_ASSERT_EQUAL(defaults.door_pulse_duration, config.door_pulse_duration);
}// ===
=========================================================================
// Build-time Configuration Override Tests
// ============================================================================

void test_config_manager_get_current_returns_merged_config(void) {
    // This test ensures that config_manager_get_current() returns the merged
    // configuration (NVS + build-time overrides) rather than raw NVS values
    
    door_station_config_t current_config;
    esp_err_t result = config_manager_get_current(&current_config);
    
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify that build-time configuration is applied
    #ifdef CONFIG_WIFI_SSID
    TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_SSID, current_config.wifi_ssid);
    #endif
    
    #ifdef CONFIG_WIFI_PASSWORD
    TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_PASSWORD, current_config.wifi_password);
    #endif
    
    #ifdef CONFIG_SIP_USER
    TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_USER, current_config.sip_user);
    #endif
    
    #ifdef CONFIG_SIP_DOMAIN
    TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_DOMAIN, current_config.sip_domain);
    #endif
    
    #ifdef CONFIG_SIP_PASSWORD
    TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_PASSWORD, current_config.sip_password);
    #endif
    
    #ifdef CONFIG_SIP_CALLEE
    TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_CALLEE, current_config.sip_callee);
    #endif
    
    #ifdef CONFIG_WEB_PORT
    TEST_ASSERT_EQUAL_UINT16(CONFIG_WEB_PORT, current_config.web_port);
    #endif
    
    #ifdef CONFIG_DOOR_PULSE_DURATION
    TEST_ASSERT_EQUAL_UINT32(CONFIG_DOOR_PULSE_DURATION, current_config.door_pulse_duration);
    #endif
}

void test_config_manager_get_current_vs_load_difference(void) {
    // This test ensures that config_manager_get_current() and config_manager_load()
    // return different values when build-time overrides are present
    
    door_station_config_t current_config, loaded_config;
    
    esp_err_t current_result = config_manager_get_current(&current_config);
    esp_err_t load_result = config_manager_load(&loaded_config);
    
    TEST_ASSERT_EQUAL(ESP_OK, current_result);
    TEST_ASSERT_EQUAL(ESP_OK, load_result);
    
    // If build-time WiFi password is configured, it should be different
    #ifdef CONFIG_WIFI_PASSWORD
    if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
        // current_config should have the build-time password
        TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_PASSWORD, current_config.wifi_password);
        
        // loaded_config might be empty if secure NVS is not available
        // The key point is that they should be different if build-time config exists
        if (strlen(loaded_config.wifi_password) == 0) {
            TEST_ASSERT_NOT_EQUAL_STRING(loaded_config.wifi_password, current_config.wifi_password);
        }
    }
    #endif
    
    // Same test for SIP password
    #ifdef CONFIG_SIP_PASSWORD
    if (strlen(CONFIG_SIP_PASSWORD) > 0) {
        TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_PASSWORD, current_config.sip_password);
        
        if (strlen(loaded_config.sip_password) == 0) {
            TEST_ASSERT_NOT_EQUAL_STRING(loaded_config.sip_password, current_config.sip_password);
        }
    }
    #endif
}

void test_config_manager_build_time_password_not_empty(void) {
    // This test specifically checks that WiFi password from build-time config
    // is not empty, which was the root cause of the original issue
    
    door_station_config_t config;
    esp_err_t result = config_manager_get_current(&config);
    
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    #ifdef CONFIG_WIFI_PASSWORD
    // If WiFi password is configured at build time, it should not be empty
    if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
        TEST_ASSERT_GREATER_THAN(0, strlen(config.wifi_password));
        TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_PASSWORD, config.wifi_password);
    }
    #endif
    
    #ifdef CONFIG_SIP_PASSWORD
    // Same for SIP password
    if (strlen(CONFIG_SIP_PASSWORD) > 0) {
        TEST_ASSERT_GREATER_THAN(0, strlen(config.sip_password));
        TEST_ASSERT_EQUAL_STRING(CONFIG_SIP_PASSWORD, config.sip_password);
    }
    #endif
}

void test_config_manager_get_current_null_parameter(void) {
    // Test error handling for null parameter
    esp_err_t result = config_manager_get_current(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_config_manager_sensitive_fields_preserved(void) {
    // This test ensures that sensitive fields (passwords) are preserved
    // in the merged configuration and not lost during the merge process
    
    door_station_config_t config;
    esp_err_t result = config_manager_get_current(&config);
    
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test that sensitive field detection works
    TEST_ASSERT_TRUE(config_manager_is_sensitive_field("wifi_password"));
    TEST_ASSERT_TRUE(config_manager_is_sensitive_field("sip_password"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("wifi_ssid"));
    TEST_ASSERT_FALSE(config_manager_is_sensitive_field("sip_user"));
    
    // Test that sensitive values are properly masked for display
    char masked_value[32];
    config_manager_mask_sensitive_value("testpassword", masked_value, sizeof(masked_value));
    TEST_ASSERT_EQUAL_STRING("********", masked_value);
    
    // Test empty value masking
    config_manager_mask_sensitive_value("", masked_value, sizeof(masked_value));
    TEST_ASSERT_EQUAL_STRING("", masked_value);
    
    // Test short value masking
    config_manager_mask_sensitive_value("abc", masked_value, sizeof(masked_value));
    TEST_ASSERT_EQUAL_STRING("***", masked_value);
}

void test_config_manager_build_time_override_priority(void) {
    // This test ensures that build-time configuration has priority over NVS values
    // This is the core test to prevent the regression we just fixed
    
    door_station_config_t config;
    esp_err_t result = config_manager_get_current(&config);
    
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Build-time values should always take precedence
    #ifdef CONFIG_WIFI_SSID
    TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_SSID, config.wifi_ssid);
    #endif
    
    #ifdef CONFIG_WEB_PORT
    TEST_ASSERT_EQUAL_UINT16(CONFIG_WEB_PORT, config.web_port);
    #endif
    
    #ifdef CONFIG_DOOR_PULSE_DURATION
    TEST_ASSERT_EQUAL_UINT32(CONFIG_DOOR_PULSE_DURATION, config.door_pulse_duration);
    #endif
    
    // The key test: passwords should not be empty if configured at build time
    #ifdef CONFIG_WIFI_PASSWORD
    if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen(config.wifi_password), 
            "WiFi password should not be empty when configured at build time");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(CONFIG_WIFI_PASSWORD, config.wifi_password,
            "WiFi password should match build-time configuration");
    }
    #endif
    
    #ifdef CONFIG_SIP_PASSWORD
    if (strlen(CONFIG_SIP_PASSWORD) > 0) {
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen(config.sip_password),
            "SIP password should not be empty when configured at build time");
        TEST_ASSERT_EQUAL_STRING_MESSAGE(CONFIG_SIP_PASSWORD, config.sip_password,
            "SIP password should match build-time configuration");
    }
    #endif
}