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