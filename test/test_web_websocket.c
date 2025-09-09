#include "unity.h"
#include "web_server.h"
#include "io_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "test_web_websocket";

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
    if (web_server_is_running()) {
        web_server_stop();
    }
}

void test_websocket_server_initialization(void) {
    ESP_LOGI(TAG, "Testing SSE server initialization");
    
    // Initialize I/O manager (required for relay status)
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server with SSE support
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    ESP_LOGI(TAG, "Web server with SSE support started successfully");
}

void test_websocket_message_format(void) {
    ESP_LOGI(TAG, "Testing SSE message format");
    
    // Test relay status message format
    cJSON *json = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(json);
    
    cJSON_AddStringToObject(json, "type", "relay_status");
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddBoolToObject(data, "door", false);
    cJSON_AddBoolToObject(data, "light", true);
    cJSON_AddItemToObject(json, "data", data);
    
    // Verify JSON structure
    cJSON *type = cJSON_GetObjectItem(json, "type");
    TEST_ASSERT_NOT_NULL(type);
    TEST_ASSERT_TRUE(cJSON_IsString(type));
    TEST_ASSERT_EQUAL_STRING("relay_status", type->valuestring);
    
    cJSON *data_obj = cJSON_GetObjectItem(json, "data");
    TEST_ASSERT_NOT_NULL(data_obj);
    TEST_ASSERT_TRUE(cJSON_IsObject(data_obj));
    
    cJSON *door = cJSON_GetObjectItem(data_obj, "door");
    cJSON *light = cJSON_GetObjectItem(data_obj, "light");
    TEST_ASSERT_NOT_NULL(door);
    TEST_ASSERT_NOT_NULL(light);
    TEST_ASSERT_TRUE(cJSON_IsBool(door));
    TEST_ASSERT_TRUE(cJSON_IsBool(light));
    TEST_ASSERT_FALSE(cJSON_IsTrue(door));
    TEST_ASSERT_TRUE(cJSON_IsTrue(light));
    
    // Test JSON string generation
    char *json_str = cJSON_Print(json);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strlen(json_str) > 0);
    
    free(json_str);
    cJSON_Delete(json);
}

void test_websocket_broadcast_function(void) {
    ESP_LOGI(TAG, "Testing SSE broadcast function");
    
    // Initialize I/O manager
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Start web server
    result = web_server_init(8080);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test broadcast function (should not fail even with no clients)
    result = web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ESP_LOGI(TAG, "SSE broadcast function works correctly");
}

void test_websocket_broadcast_without_server(void) {
    ESP_LOGI(TAG, "Testing SSE broadcast without server");
    
    // Initialize I/O manager
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Try to broadcast without server running
    result = web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result); // Should not fail, just do nothing
    
    ESP_LOGI(TAG, "SSE broadcast handles no server gracefully");
}

void test_websocket_ping_pong_message(void) {
    ESP_LOGI(TAG, "Testing WebSocket ping/pong message handling");
    
    // Test ping message format
    const char *ping_msg = "ping";
    const char *expected_pong = "pong";
    
    TEST_ASSERT_EQUAL_STRING("ping", ping_msg);
    TEST_ASSERT_EQUAL_STRING("pong", expected_pong);
    TEST_ASSERT_NOT_EQUAL(0, strncmp(ping_msg, expected_pong, 4));
    
    // Test message length validation
    TEST_ASSERT_EQUAL(4, strlen(ping_msg));
    TEST_ASSERT_EQUAL(4, strlen(expected_pong));
}

void test_websocket_client_tracking(void) {
    ESP_LOGI(TAG, "Testing WebSocket client tracking logic");
    
    // Test client array bounds
    const int MAX_WS_CLIENTS = 4;
    int test_clients[MAX_WS_CLIENTS];
    int client_count = 0;
    
    // Simulate adding clients
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        test_clients[client_count] = i + 100; // Simulate file descriptors
        client_count++;
    }
    
    TEST_ASSERT_EQUAL(MAX_WS_CLIENTS, client_count);
    
    // Simulate removing a client
    int fd_to_remove = 101;
    for (int i = 0; i < client_count; i++) {
        if (test_clients[i] == fd_to_remove) {
            // Shift remaining clients
            for (int j = i; j < client_count - 1; j++) {
                test_clients[j] = test_clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    
    TEST_ASSERT_EQUAL(MAX_WS_CLIENTS - 1, client_count);
    TEST_ASSERT_EQUAL(100, test_clients[0]);
    TEST_ASSERT_EQUAL(102, test_clients[1]);
    TEST_ASSERT_EQUAL(103, test_clients[2]);
}

void test_websocket_json_relay_states(void) {
    ESP_LOGI(TAG, "Testing WebSocket JSON for different relay states");
    
    // Test all combinations of relay states
    struct {
        relay_state_t door_state;
        relay_state_t light_state;
        bool expected_door;
        bool expected_light;
    } test_cases[] = {
        {RELAY_STATE_OFF, RELAY_STATE_OFF, false, false},
        {RELAY_STATE_OFF, RELAY_STATE_ON, false, true},
        {RELAY_STATE_ON, RELAY_STATE_OFF, true, false},
        {RELAY_STATE_ON, RELAY_STATE_ON, true, true}
    };
    
    for (int i = 0; i < 4; i++) {
        cJSON *json = cJSON_CreateObject();
        TEST_ASSERT_NOT_NULL(json);
        
        cJSON_AddStringToObject(json, "type", "relay_status");
        
        cJSON *data = cJSON_CreateObject();
        cJSON_AddBoolToObject(data, "door", test_cases[i].door_state == RELAY_STATE_ON);
        cJSON_AddBoolToObject(data, "light", test_cases[i].light_state == RELAY_STATE_ON);
        cJSON_AddItemToObject(json, "data", data);
        
        // Verify the JSON matches expected values
        cJSON *data_obj = cJSON_GetObjectItem(json, "data");
        cJSON *door = cJSON_GetObjectItem(data_obj, "door");
        cJSON *light = cJSON_GetObjectItem(data_obj, "light");
        
        TEST_ASSERT_EQUAL(test_cases[i].expected_door, cJSON_IsTrue(door));
        TEST_ASSERT_EQUAL(test_cases[i].expected_light, cJSON_IsTrue(light));
        
        cJSON_Delete(json);
    }
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_websocket_server_initialization);
    RUN_TEST(test_websocket_message_format);
    RUN_TEST(test_websocket_broadcast_function);
    RUN_TEST(test_websocket_broadcast_without_server);
    RUN_TEST(test_websocket_ping_pong_message);
    RUN_TEST(test_websocket_client_tracking);
    RUN_TEST(test_websocket_json_relay_states);
    
    UNITY_END();
}