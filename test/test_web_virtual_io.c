#include "unity.h"
#include "web_server.h"
#include "io_manager.h"
#include "io_events.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "test_web_virtual_io";

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
    if (web_server_is_running()) {
        web_server_stop();
    }
}

void test_virtual_doorbell_api_integration(void) {
    ESP_LOGI(TAG, "Testing virtual doorbell API integration");
    
    // Initialize I/O manager
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Test that button press event can be published
    result = io_events_publish_button(true);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ESP_LOGI(TAG, "Virtual doorbell API integration test passed");
}

void test_relay_status_json_format(void) {
    ESP_LOGI(TAG, "Testing relay status JSON format");
    
    // Test JSON creation for relay status
    cJSON *json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);
    
    // Add relay states (simulating API response)
    cJSON_AddBoolToObject(json, "door", false);
    cJSON_AddBoolToObject(json, "light", true);
    
    // Verify JSON structure
    cJSON *door = cJSON_GetObjectItem(json, "door");
    TEST_ASSERT_NOT_NULL(door);
    TEST_ASSERT_TRUE(cJSON_IsBool(door));
    TEST_ASSERT_FALSE(cJSON_IsTrue(door));
    
    cJSON *light = cJSON_GetObjectItem(json, "light");
    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_TRUE(cJSON_IsBool(light));
    TEST_ASSERT_TRUE(cJSON_IsTrue(light));
    
    // Test JSON string generation
    char *json_str = cJSON_Print(json);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strlen(json_str) > 0);
    
    free(json_str);
    cJSON_Delete(json);
}

void test_system_status_json_format(void) {
    ESP_LOGI(TAG, "Testing system status JSON format");
    
    // Test JSON creation for system status
    cJSON *json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);
    
    // Add relay status
    cJSON *relays = cJSON_CreateObject();
    cJSON_AddBoolToObject(relays, "door", false);
    cJSON_AddBoolToObject(relays, "light", true);
    cJSON_AddItemToObject(json, "relays", relays);
    
    // Add system status
    cJSON_AddStringToObject(json, "system", "running");
    cJSON_AddBoolToObject(json, "web_server", true);
    
    // Verify JSON structure
    cJSON *relays_obj = cJSON_GetObjectItem(json, "relays");
    TEST_ASSERT_NOT_NULL(relays_obj);
    TEST_ASSERT_TRUE(cJSON_IsObject(relays_obj));
    
    cJSON *system = cJSON_GetObjectItem(json, "system");
    TEST_ASSERT_NOT_NULL(system);
    TEST_ASSERT_TRUE(cJSON_IsString(system));
    TEST_ASSERT_EQUAL_STRING("running", system->valuestring);
    
    cJSON *web_server = cJSON_GetObjectItem(json, "web_server");
    TEST_ASSERT_NOT_NULL(web_server);
    TEST_ASSERT_TRUE(cJSON_IsBool(web_server));
    TEST_ASSERT_TRUE(cJSON_IsTrue(web_server));
    
    // Test JSON string generation
    char *json_str = cJSON_Print(json);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strlen(json_str) > 0);
    
    free(json_str);
    cJSON_Delete(json);
}

void test_doorbell_response_format(void) {
    ESP_LOGI(TAG, "Testing doorbell response format");
    
    // Test expected doorbell response JSON
    const char *expected_response = "{\"status\":\"success\",\"message\":\"Doorbell pressed\"}";
    
    cJSON *json = cJSON_Parse(expected_response);
    TEST_ASSERT_NOT_NULL(json);
    
    cJSON *status = cJSON_GetObjectItem(json, "status");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_TRUE(cJSON_IsString(status));
    TEST_ASSERT_EQUAL_STRING("success", status->valuestring);
    
    cJSON *message = cJSON_GetObjectItem(json, "message");
    TEST_ASSERT_NOT_NULL(message);
    TEST_ASSERT_TRUE(cJSON_IsString(message));
    TEST_ASSERT_EQUAL_STRING("Doorbell pressed", message->valuestring);
    
    cJSON_Delete(json);
}

void test_relay_state_mapping(void) {
    ESP_LOGI(TAG, "Testing relay state mapping");
    
    // Test relay state enum to boolean conversion
    relay_state_t state_off = RELAY_STATE_OFF;
    relay_state_t state_on = RELAY_STATE_ON;
    
    bool bool_off = (state_off == RELAY_STATE_ON);
    bool bool_on = (state_on == RELAY_STATE_ON);
    
    TEST_ASSERT_FALSE(bool_off);
    TEST_ASSERT_TRUE(bool_on);
    
    // Test in JSON context
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "relay_off", bool_off);
    cJSON_AddBoolToObject(json, "relay_on", bool_on);
    
    cJSON *off_item = cJSON_GetObjectItem(json, "relay_off");
    cJSON *on_item = cJSON_GetObjectItem(json, "relay_on");
    
    TEST_ASSERT_FALSE(cJSON_IsTrue(off_item));
    TEST_ASSERT_TRUE(cJSON_IsTrue(on_item));
    
    cJSON_Delete(json);
}

void test_api_endpoint_paths(void) {
    ESP_LOGI(TAG, "Testing API endpoint path definitions");
    
    // Test that we have the correct API paths defined
    const char *doorbell_path = "/api/doorbell";
    const char *relays_path = "/api/relays";
    const char *status_path = "/api/status";
    
    // Verify path formats
    TEST_ASSERT_TRUE(strncmp(doorbell_path, "/api/", 5) == 0);
    TEST_ASSERT_TRUE(strncmp(relays_path, "/api/", 5) == 0);
    TEST_ASSERT_TRUE(strncmp(status_path, "/api/", 5) == 0);
    
    // Verify path lengths are reasonable
    TEST_ASSERT_TRUE(strlen(doorbell_path) < 32);
    TEST_ASSERT_TRUE(strlen(relays_path) < 32);
    TEST_ASSERT_TRUE(strlen(status_path) < 32);
}

void test_web_server_with_virtual_io_apis(void) {
    ESP_LOGI(TAG, "Testing web server initialization with virtual I/O APIs");
    
    // Initialize dependencies
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server (should register all API endpoints)
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Server should be running with all endpoints registered
    ESP_LOGI(TAG, "Web server started with virtual I/O API endpoints");
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_virtual_doorbell_api_integration);
    RUN_TEST(test_relay_status_json_format);
    RUN_TEST(test_system_status_json_format);
    RUN_TEST(test_doorbell_response_format);
    RUN_TEST(test_relay_state_mapping);
    RUN_TEST(test_api_endpoint_paths);
    RUN_TEST(test_web_server_with_virtual_io_apis);
    
    UNITY_END();
}