#include "unity.h"
#include "app_controller.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "test_app_controller";

void setUp(void)
{
    // Reset any global state before each test
}

void tearDown(void)
{
    // Clean up after each test
}

void test_app_controller_init_success(void)
{
    ESP_LOGI(TAG, "Testing app controller initialization");
    
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test double initialization
    result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result); // Should handle gracefully
}

void test_app_controller_get_system_state(void)
{
    ESP_LOGI(TAG, "Testing system state retrieval");
    
    // Initialize controller first
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Check initial state values
    TEST_ASSERT_EQUAL(APP_STATE_INITIALIZING, state.app_state);
    TEST_ASSERT_EQUAL(SIP_STATE_IDLE, state.sip_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.door_relay_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.light_relay_state);
    TEST_ASSERT_FALSE(state.button_pressed);
    TEST_ASSERT_FALSE(state.sip_registered);
    TEST_ASSERT_EQUAL(0, state.error_count);
    
    // Test with NULL pointer
    result = app_controller_get_system_state(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

void test_app_controller_state_transitions(void)
{
    ESP_LOGI(TAG, "Testing application state transitions");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test state transitions
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state);
    
    // Transition to calling state
    result = app_controller_set_app_state(APP_STATE_CALLING);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    
    // Transition to connected state
    result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CONNECTED, state.app_state);
    
    // Transition to error state
    result = app_controller_set_app_state(APP_STATE_ERROR);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_ERROR, state.app_state);
}

void test_app_controller_button_press_handling(void)
{
    ESP_LOGI(TAG, "Testing button press handling");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Set to idle state
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Handle button press in idle state - should transition to calling
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    TEST_ASSERT_TRUE(state.button_pressed);
    TEST_ASSERT_NOT_EQUAL(0, state.call_start_time);
    
    // Handle button press in calling state - should be ignored
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state); // Should remain in calling state
    
    // Transition to connected and test button press
    result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state); // Should end call and go to idle
}

void test_app_controller_dtmf_handling(void)
{
    ESP_LOGI(TAG, "Testing DTMF handling");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test DTMF in non-connected state - should fail
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    // Set to connected state
    result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test valid DTMF digits
    result = app_controller_handle_dtmf('1'); // Door relay pulse
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_dtmf('2'); // Light relay toggle
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_dtmf('#'); // Status request
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test call termination with '*'
    result = app_controller_handle_dtmf('*');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state); // Should end call
    
    // Test unknown DTMF digit
    result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_dtmf('9'); // Unknown digit
    TEST_ASSERT_EQUAL(ESP_OK, result); // Should handle gracefully
}

void test_app_controller_sip_state_handling(void)
{
    ESP_LOGI(TAG, "Testing SIP state change handling");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test SIP registration
    result = app_controller_handle_sip_state_change(SIP_STATE_REGISTERED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, state.sip_state);
    TEST_ASSERT_TRUE(state.sip_registered);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state); // Should transition from initializing to idle
    
    // Test SIP calling
    result = app_controller_handle_sip_state_change(SIP_STATE_CALLING);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_CALLING, state.sip_state);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    
    // Test SIP connected
    result = app_controller_handle_sip_state_change(SIP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_CONNECTED, state.sip_state);
    TEST_ASSERT_EQUAL(APP_STATE_CONNECTED, state.app_state);
    
    // Test SIP idle (call ended)
    result = app_controller_handle_sip_state_change(SIP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_IDLE, state.sip_state);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state);
    TEST_ASSERT_FALSE(state.sip_registered);
    
    // Test SIP error
    result = app_controller_handle_sip_state_change(SIP_STATE_ERROR);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_ERROR, state.sip_state);
    TEST_ASSERT_EQUAL(APP_STATE_ERROR, state.app_state);
    TEST_ASSERT_EQUAL(1, state.error_count);
}

void test_app_controller_relay_state_handling(void)
{
    ESP_LOGI(TAG, "Testing relay state change handling");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test door relay state change
    result = app_controller_handle_relay_state_change(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state.door_relay_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.light_relay_state); // Should remain unchanged
    
    // Test light relay state change
    result = app_controller_handle_relay_state_change(RELAY_LIGHT, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state.door_relay_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state.light_relay_state);
    
    // Test turning relays off
    result = app_controller_handle_relay_state_change(RELAY_DOOR, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_relay_state_change(RELAY_LIGHT, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.door_relay_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.light_relay_state);
}

void test_app_controller_error_handling(void)
{
    ESP_LOGI(TAG, "Testing error handling");
    
    // Initialize controller
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test error handling
    result = app_controller_handle_error(123, "Test error message");
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_ERROR, state.app_state);
    TEST_ASSERT_EQUAL(1, state.error_count);
    TEST_ASSERT_EQUAL_STRING("Test error message", state.last_error);
    
    // Test error with NULL message
    result = app_controller_handle_error(456, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(2, state.error_count);
    TEST_ASSERT_TRUE(strstr(state.last_error, "Error code: 456") != NULL);
    
    // Test error count reset
    result = app_controller_reset_error_count();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(0, state.error_count);
    TEST_ASSERT_EQUAL_STRING("", state.last_error);
}

void test_app_controller_utility_functions(void)
{
    ESP_LOGI(TAG, "Testing utility functions");
    
    // Test state string conversion
    TEST_ASSERT_EQUAL_STRING("INITIALIZING", app_controller_get_state_string(APP_STATE_INITIALIZING));
    TEST_ASSERT_EQUAL_STRING("IDLE", app_controller_get_state_string(APP_STATE_IDLE));
    TEST_ASSERT_EQUAL_STRING("CALLING", app_controller_get_state_string(APP_STATE_CALLING));
    TEST_ASSERT_EQUAL_STRING("CONNECTED", app_controller_get_state_string(APP_STATE_CONNECTED));
    TEST_ASSERT_EQUAL_STRING("ERROR", app_controller_get_state_string(APP_STATE_ERROR));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", app_controller_get_state_string((app_state_t)999));
    
    // Test system ready check before initialization
    bool ready = app_controller_is_system_ready();
    TEST_ASSERT_FALSE(ready);
    
    // Initialize and test again
    esp_err_t result = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ready = app_controller_is_system_ready();
    TEST_ASSERT_FALSE(ready); // Should be false in initializing state
    
    // Transition to idle and test
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    ready = app_controller_is_system_ready();
    TEST_ASSERT_TRUE(ready); // Should be true in idle state
    
    // Test uptime
    uint32_t uptime = app_controller_get_uptime();
    TEST_ASSERT_GREATER_OR_EQUAL(0, uptime);
    
    // Wait a bit and check uptime increased
    vTaskDelay(pdMS_TO_TICKS(100));
    uint32_t uptime2 = app_controller_get_uptime();
    TEST_ASSERT_GREATER_OR_EQUAL(uptime, uptime2);
}

void test_app_controller_uninitialized_operations(void)
{
    ESP_LOGI(TAG, "Testing operations on uninitialized controller");
    
    // Test operations before initialization
    system_state_t state;
    esp_err_t result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_handle_sip_state_change(SIP_STATE_REGISTERED);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_handle_relay_state_change(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_handle_error(123, "Test error");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    result = app_controller_reset_error_count();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, result);
    
    uint32_t uptime = app_controller_get_uptime();
    TEST_ASSERT_EQUAL(0, uptime);
    
    bool ready = app_controller_is_system_ready();
    TEST_ASSERT_FALSE(ready);
}

// Test runner
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_app_controller_init_success);
    RUN_TEST(test_app_controller_get_system_state);
    RUN_TEST(test_app_controller_state_transitions);
    RUN_TEST(test_app_controller_button_press_handling);
    RUN_TEST(test_app_controller_dtmf_handling);
    RUN_TEST(test_app_controller_sip_state_handling);
    RUN_TEST(test_app_controller_relay_state_handling);
    RUN_TEST(test_app_controller_error_handling);
    RUN_TEST(test_app_controller_utility_functions);
    RUN_TEST(test_app_controller_uninitialized_operations);
    
    UNITY_END();
}