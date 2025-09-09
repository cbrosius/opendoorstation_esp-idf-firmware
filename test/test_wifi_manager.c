#include "unity.h"
#include "wifi_manager.h"
#include "config_manager.h"
#include "mocks/mock_esp_wifi.h"
#include "mocks/mock_esp_netif.h"
#include "mocks/mock_esp_event.h"
#include <string.h>

static const char *TAG = "test_wifi_manager";

void setUp(void) {
    // Initialize mocks
    mock_esp_wifi_init();
    mock_esp_netif_init();
    mock_esp_event_init();
}

void tearDown(void) {
    // Clean up mocks
    mock_esp_wifi_cleanup();
    mock_esp_netif_cleanup();
    mock_esp_event_cleanup();
}

void test_wifi_manager_init_success(void) {
    esp_err_t result = wifi_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

void test_wifi_manager_connect_with_password(void) {
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test connection with password
    const char* test_ssid = "TestNetwork";
    const char* test_password = "testpassword123";
    
    esp_err_t result = wifi_manager_connect(test_ssid, test_password);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify that the mock received the correct credentials
    wifi_config_t* last_config = mock_esp_wifi_get_last_config();
    TEST_ASSERT_NOT_NULL(last_config);
    TEST_ASSERT_EQUAL_STRING(test_ssid, (char*)last_config->sta.ssid);
    TEST_ASSERT_EQUAL_STRING(test_password, (char*)last_config->sta.password);
}

void test_wifi_manager_connect_empty_password_fails(void) {
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test connection with empty password - should still work but log warning
    const char* test_ssid = "TestNetwork";
    const char* empty_password = "";
    
    esp_err_t result = wifi_manager_connect(test_ssid, empty_password);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify that the mock received empty password
    wifi_config_t* last_config = mock_esp_wifi_get_last_config();
    TEST_ASSERT_NOT_NULL(last_config);
    TEST_ASSERT_EQUAL_STRING(test_ssid, (char*)last_config->sta.ssid);
    TEST_ASSERT_EQUAL_STRING("", (char*)last_config->sta.password);
}

void test_wifi_manager_connect_null_password(void) {
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test connection with NULL password
    const char* test_ssid = "TestNetwork";
    
    esp_err_t result = wifi_manager_connect(test_ssid, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify that the mock received empty password
    wifi_config_t* last_config = mock_esp_wifi_get_last_config();
    TEST_ASSERT_NOT_NULL(last_config);
    TEST_ASSERT_EQUAL_STRING(test_ssid, (char*)last_config->sta.ssid);
    TEST_ASSERT_EQUAL_STRING("", (char*)last_config->sta.password);
}

void test_wifi_manager_connect_invalid_ssid(void) {
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test connection with NULL SSID
    esp_err_t result = wifi_manager_connect(NULL, "password");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
    
    // Test connection with empty SSID
    result = wifi_manager_connect("", "password");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_wifi_manager_get_info(void) {
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    wifi_info_t info;
    esp_err_t result = wifi_manager_get_info(&info);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Initially should be disconnected
    TEST_ASSERT_EQUAL(WIFI_STATE_DISCONNECTED, info.state);
    TEST_ASSERT_EQUAL_STRING("", info.ip_address);
}

void test_wifi_manager_state_strings(void) {
    TEST_ASSERT_EQUAL_STRING("DISCONNECTED", wifi_manager_get_state_string(WIFI_STATE_DISCONNECTED));
    TEST_ASSERT_EQUAL_STRING("CONNECTING", wifi_manager_get_state_string(WIFI_STATE_CONNECTING));
    TEST_ASSERT_EQUAL_STRING("CONNECTED", wifi_manager_get_state_string(WIFI_STATE_CONNECTED));
    TEST_ASSERT_EQUAL_STRING("ERROR", wifi_manager_get_state_string(WIFI_STATE_ERROR));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", wifi_manager_get_state_string((wifi_state_t)999));
}

// ============================================================================
// Integration Tests with Config Manager
// ============================================================================

void test_wifi_manager_integration_with_config_manager(void) {
    // This test ensures that WiFi manager receives the correct password
    // from the config manager, preventing the regression we just fixed
    
    // Initialize config manager first
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Get current merged configuration
    door_station_config_t config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_get_current(&config));
    
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test that WiFi manager can connect with the config from config manager
    if (strlen(config.wifi_ssid) > 0) {
        esp_err_t result = wifi_manager_connect(config.wifi_ssid, config.wifi_password);
        TEST_ASSERT_EQUAL(ESP_OK, result);
        
        // Verify that the WiFi manager received the correct credentials
        wifi_config_t* last_config = mock_esp_wifi_get_last_config();
        TEST_ASSERT_NOT_NULL(last_config);
        TEST_ASSERT_EQUAL_STRING(config.wifi_ssid, (char*)last_config->sta.ssid);
        TEST_ASSERT_EQUAL_STRING(config.wifi_password, (char*)last_config->sta.password);
        
        // Key test: if build-time password is configured, it should not be empty
        #ifdef CONFIG_WIFI_PASSWORD
        if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
            TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen((char*)last_config->sta.password),
                "WiFi password should not be empty when configured at build time");
            TEST_ASSERT_EQUAL_STRING_MESSAGE(CONFIG_WIFI_PASSWORD, (char*)last_config->sta.password,
                "WiFi password should match build-time configuration");
        }
        #endif
    }
}

void test_wifi_manager_password_length_validation(void) {
    // This test specifically validates that password length is correctly detected
    // This was part of the original issue where password length was reported as 0
    
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Test with various password lengths
    const char* test_cases[] = {
        "",                    // Empty password
        "a",                   // 1 character
        "abc",                 // 3 characters
        "password123",         // 11 characters
        "verylongpassword123456789", // Long password
    };
    
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const char* password = test_cases[i];
        esp_err_t result = wifi_manager_connect("TestSSID", password);
        TEST_ASSERT_EQUAL(ESP_OK, result);
        
        wifi_config_t* last_config = mock_esp_wifi_get_last_config();
        TEST_ASSERT_NOT_NULL(last_config);
        TEST_ASSERT_EQUAL_STRING(password, (char*)last_config->sta.password);
        TEST_ASSERT_EQUAL_INT(strlen(password), strlen((char*)last_config->sta.password));
    }
}

void test_wifi_manager_build_time_config_integration(void) {
    // This is the key regression test to ensure build-time WiFi configuration
    // is properly passed to the WiFi manager
    
    #ifdef CONFIG_WIFI_SSID
    #ifdef CONFIG_WIFI_PASSWORD
    
    // Initialize config manager to apply build-time config
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Get the merged configuration
    door_station_config_t config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_get_current(&config));
    
    // Verify build-time config is applied
    TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_SSID, config.wifi_ssid);
    if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
        TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_PASSWORD, config.wifi_password);
        TEST_ASSERT_GREATER_THAN(0, strlen(config.wifi_password));
    }
    
    // Initialize WiFi manager
    TEST_ASSERT_EQUAL(ESP_OK, wifi_manager_init());
    
    // Connect using the configuration
    esp_err_t result = wifi_manager_connect(config.wifi_ssid, config.wifi_password);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify WiFi manager received the correct build-time configuration
    wifi_config_t* last_config = mock_esp_wifi_get_last_config();
    TEST_ASSERT_NOT_NULL(last_config);
    TEST_ASSERT_EQUAL_STRING(CONFIG_WIFI_SSID, (char*)last_config->sta.ssid);
    
    if (strlen(CONFIG_WIFI_PASSWORD) > 0) {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(CONFIG_WIFI_PASSWORD, (char*)last_config->sta.password,
            "WiFi manager should receive build-time password, not empty string");
        TEST_ASSERT_GREATER_THAN_MESSAGE(0, strlen((char*)last_config->sta.password),
            "WiFi password length should be greater than 0 when build-time password is configured");
    }
    
    #endif
    #endif
}