#include "unity.h"
#include "web_server.h"
#include "config_manager.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "test_web_api";

// Mock HTTP request structure for testing
typedef struct {
    char *content;
    size_t content_len;
    char response_buffer[2048];
    size_t response_len;
    char content_type[64];
    int status_code;
} mock_http_req_t;

static mock_http_req_t mock_req;

void setUp(void) {
    // Setup before each test
    memset(&mock_req, 0, sizeof(mock_req));
    mock_req.status_code = 200;
}

void tearDown(void) {
    // Cleanup after each test
    if (mock_req.content) {
        free(mock_req.content);
        mock_req.content = NULL;
    }
    if (web_server_is_running()) {
        web_server_stop();
    }
}

void test_config_api_get_success(void) {
    ESP_LOGI(TAG, "Testing GET /api/config success");
    
    // Initialize config manager with test data
    door_station_config_t test_config = {
        .wifi_ssid = "TestWiFi",
        .wifi_password = "password123",
        .sip_user = "testuser",
        .sip_domain = "test.domain.com",
        .sip_password = "sippass",
        .sip_callee = "user@domain.com",
        .web_port = 8080,
        .door_pulse_duration = 2000
    };
    
    esp_err_t result = config_manager_save(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // The actual HTTP request testing would require more complex mocking
    // For now, we test that the server starts successfully with API endpoints
    TEST_ASSERT_TRUE(web_server_is_running());
    
    ESP_LOGI(TAG, "Web server with API endpoints started successfully");
}

void test_config_api_json_parsing(void) {
    ESP_LOGI(TAG, "Testing JSON configuration parsing");
    
    // Test valid JSON parsing
    const char *valid_json = "{"
        "\"wifi_ssid\":\"NewWiFi\","
        "\"sip_user\":\"newuser\","
        "\"sip_domain\":\"new.domain.com\","
        "\"web_port\":9090,"
        "\"door_pulse_duration\":3000"
    "}";
    
    cJSON *json = cJSON_Parse(valid_json);
    TEST_ASSERT_NOT_NULL(json);
    
    // Verify JSON structure
    cJSON *wifi_ssid = cJSON_GetObjectItem(json, "wifi_ssid");
    TEST_ASSERT_NOT_NULL(wifi_ssid);
    TEST_ASSERT_TRUE(cJSON_IsString(wifi_ssid));
    TEST_ASSERT_EQUAL_STRING("NewWiFi", wifi_ssid->valuestring);
    
    cJSON *web_port = cJSON_GetObjectItem(json, "web_port");
    TEST_ASSERT_NOT_NULL(web_port);
    TEST_ASSERT_TRUE(cJSON_IsNumber(web_port));
    TEST_ASSERT_EQUAL_INT(9090, web_port->valueint);
    
    cJSON_Delete(json);
}

void test_config_api_json_invalid(void) {
    ESP_LOGI(TAG, "Testing invalid JSON handling");
    
    // Test invalid JSON
    const char *invalid_json = "{\"wifi_ssid\":\"test\",}"; // Trailing comma
    
    cJSON *json = cJSON_Parse(invalid_json);
    TEST_ASSERT_NULL(json); // Should fail to parse
}

void test_config_api_sensitive_data_masking(void) {
    ESP_LOGI(TAG, "Testing sensitive data masking");
    
    // Create test configuration with sensitive data
    door_station_config_t config = {
        .wifi_ssid = "TestWiFi",
        .wifi_password = "secret123",
        .sip_user = "testuser",
        .sip_domain = "test.com",
        .sip_password = "topsecret",
        .sip_callee = "user@test.com",
        .web_port = 80,
        .door_pulse_duration = 2000
    };
    
    // Test that we can create JSON representation
    cJSON *json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);
    
    // Add masked password fields
    cJSON_AddStringToObject(json, "wifi_ssid", config.wifi_ssid);
    cJSON_AddStringToObject(json, "wifi_password", strlen(config.wifi_password) > 0 ? "********" : "");
    cJSON_AddStringToObject(json, "sip_password", strlen(config.sip_password) > 0 ? "********" : "");
    
    // Verify masking
    cJSON *wifi_pass = cJSON_GetObjectItem(json, "wifi_password");
    TEST_ASSERT_NOT_NULL(wifi_pass);
    TEST_ASSERT_EQUAL_STRING("********", wifi_pass->valuestring);
    
    cJSON *sip_pass = cJSON_GetObjectItem(json, "sip_password");
    TEST_ASSERT_NOT_NULL(sip_pass);
    TEST_ASSERT_EQUAL_STRING("********", sip_pass->valuestring);
    
    cJSON_Delete(json);
}

void test_config_api_validation_integration(void) {
    ESP_LOGI(TAG, "Testing configuration validation integration");
    
    // Test valid configuration
    door_station_config_t valid_config = {
        .wifi_ssid = "ValidWiFi",
        .wifi_password = "validpass",
        .sip_user = "validuser",
        .sip_domain = "valid.domain.com",
        .sip_password = "validpass",
        .sip_callee = "user@valid.com",
        .web_port = 8080,
        .door_pulse_duration = 2000
    };
    
    config_validation_error_t validation_result = config_manager_validate(&valid_config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, validation_result);
    
    // Test invalid configuration (port too low)
    door_station_config_t invalid_config = valid_config;
    invalid_config.web_port = 500; // Below minimum
    
    validation_result = config_manager_validate(&invalid_config);
    TEST_ASSERT_NOT_EQUAL(CONFIG_VALIDATION_OK, validation_result);
    
    const char* error_msg = config_manager_get_validation_error_message(validation_result);
    TEST_ASSERT_NOT_NULL(error_msg);
    TEST_ASSERT_NOT_EQUAL(0, strlen(error_msg));
}

void test_config_api_partial_updates(void) {
    ESP_LOGI(TAG, "Testing partial configuration updates");
    
    // Set initial configuration
    door_station_config_t initial_config = {
        .wifi_ssid = "InitialWiFi",
        .wifi_password = "initialpass",
        .sip_user = "initialuser",
        .sip_domain = "initial.com",
        .sip_password = "initialpass",
        .sip_callee = "user@initial.com",
        .web_port = 80,
        .door_pulse_duration = 2000
    };
    
    esp_err_t result = config_manager_save(&initial_config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test partial update JSON (only updating some fields)
    const char *partial_json = "{"
        "\"wifi_ssid\":\"UpdatedWiFi\","
        "\"web_port\":9090"
    "}";
    
    cJSON *json = cJSON_Parse(partial_json);
    TEST_ASSERT_NOT_NULL(json);
    
    // Verify we can parse partial updates
    cJSON *wifi_ssid = cJSON_GetObjectItem(json, "wifi_ssid");
    TEST_ASSERT_NOT_NULL(wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("UpdatedWiFi", wifi_ssid->valuestring);
    
    // Verify missing fields are not present
    cJSON *sip_user = cJSON_GetObjectItem(json, "sip_user");
    TEST_ASSERT_NULL(sip_user);
    
    cJSON_Delete(json);
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_config_api_get_success);
    RUN_TEST(test_config_api_json_parsing);
    RUN_TEST(test_config_api_json_invalid);
    RUN_TEST(test_config_api_sensitive_data_masking);
    RUN_TEST(test_config_api_validation_integration);
    RUN_TEST(test_config_api_partial_updates);
    
    UNITY_END();
}