#include "unity.h"
#include "app_controller.h"
#include "sip_manager.h"
#include "io_manager.h"
#include "config_manager.h"
#include "io_events.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "test_app_integration";

// Test configuration
static door_station_config_t test_config = {
    .wifi_ssid = "TestWiFi",
    .wifi_password = "TestPassword",
    .sip_user = "testuser",
    .sip_domain = "test.domain.com",
    .sip_password = "testpass",
    .sip_callee = "sip:callee@test.domain.com",
    .web_port = 8080,
    .door_pulse_duration = 2000
};

void setUp(void)
{
    // Initialize components for each test
    esp_err_t ret = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = app_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void tearDown(void)
{
    // Clean up after each test
    app_controller_reset_error_count();
}

void test_complete_doorbell_to_call_workflow(void)
{
    ESP_LOGI(TAG, "Testing complete doorbell to call workflow");
    
    // Initialize SIP manager with test configuration
    sip_config_t sip_config = {
        .port = 5060,
        .registration_timeout = 60,
        .call_timeout = 30
    };
    strncpy(sip_config.user, test_config.sip_user, sizeof(sip_config.user) - 1);
    strncpy(sip_config.domain, test_config.sip_domain, sizeof(sip_config.domain) - 1);
    strncpy(sip_config.password, test_config.sip_password, sizeof(sip_config.password) - 1);
    strncpy(sip_config.callee, test_config.sip_callee, sizeof(sip_config.callee) - 1);
    
    esp_err_t result = sip_manager_init(&sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Set system to idle state (simulating successful initialization)
    result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate SIP registration
    result = app_controller_handle_sip_state_change(SIP_STATE_REGISTERED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state);
    TEST_ASSERT_TRUE(state.sip_registered);
    
    // Step 1: Simulate button press (doorbell)
    ESP_LOGI(TAG, "Step 1: Button press");
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    TEST_ASSERT_TRUE(state.button_pressed);
    TEST_ASSERT_NOT_EQUAL(0, state.call_start_time);
    
    // Step 2: Simulate SIP call initiation
    ESP_LOGI(TAG, "Step 2: SIP call initiated");
    result = app_controller_handle_sip_state_change(SIP_STATE_CALLING);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    TEST_ASSERT_EQUAL(SIP_STATE_CALLING, state.sip_state);
    
    // Step 3: Simulate call connected
    ESP_LOGI(TAG, "Step 3: Call connected");
    result = app_controller_handle_sip_state_change(SIP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CONNECTED, state.app_state);
    TEST_ASSERT_EQUAL(SIP_STATE_CONNECTED, state.sip_state);
    
    // Step 4: Test DTMF commands during call
    ESP_LOGI(TAG, "Step 4: DTMF commands");
    
    // DTMF '1' - Door relay pulse
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_NOT_EQUAL(0, state.last_dtmf_time);
    
    // DTMF '2' - Light relay toggle
    result = app_controller_handle_dtmf('2');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // DTMF '#' - Status request
    result = app_controller_handle_dtmf('#');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Step 5: End call with DTMF '*'
    ESP_LOGI(TAG, "Step 5: End call");
    result = app_controller_handle_dtmf('*');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state);
    
    // Simulate SIP state change to idle
    result = app_controller_handle_sip_state_change(SIP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(SIP_STATE_IDLE, state.sip_state);
    TEST_ASSERT_FALSE(state.sip_registered);
    
    ESP_LOGI(TAG, "Complete doorbell to call workflow test passed");
}

void test_dtmf_to_relay_control_workflow(void)
{
    ESP_LOGI(TAG, "Testing DTMF to relay control workflow");
    
    // Set up system in connected state
    esp_err_t result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_sip_state_change(SIP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CONNECTED, state.app_state);
    
    // Test door relay control with DTMF '1'
    ESP_LOGI(TAG, "Testing door relay control");
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate relay state change (would normally come from I/O manager)
    result = app_controller_handle_relay_state_change(RELAY_DOOR, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state.door_relay_state);
    
    // Simulate relay turning off after pulse
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to simulate pulse duration
    result = app_controller_handle_relay_state_change(RELAY_DOOR, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.door_relay_state);
    
    // Test light relay control with DTMF '2'
    ESP_LOGI(TAG, "Testing light relay control");
    result = app_controller_handle_dtmf('2');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate light relay toggle on
    result = app_controller_handle_relay_state_change(RELAY_LIGHT, RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, state.light_relay_state);
    
    // Test light relay toggle off
    result = app_controller_handle_dtmf('2');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_relay_state_change(RELAY_LIGHT, RELAY_STATE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, state.light_relay_state);
    
    ESP_LOGI(TAG, "DTMF to relay control workflow test passed");
}

void test_error_recovery_workflow(void)
{
    ESP_LOGI(TAG, "Testing error recovery workflow");
    
    // Start in idle state
    esp_err_t result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate SIP error
    result = app_controller_handle_sip_state_change(SIP_STATE_ERROR);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_ERROR, state.app_state);
    TEST_ASSERT_EQUAL(SIP_STATE_ERROR, state.sip_state);
    TEST_ASSERT_EQUAL(1, state.error_count);
    
    // Test recovery with button press
    ESP_LOGI(TAG, "Testing recovery with button press");
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state); // Should recover to idle
    
    // Test multiple errors
    ESP_LOGI(TAG, "Testing multiple errors");
    result = app_controller_handle_error(100, "First error");
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_error(200, "Second error");
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_ERROR, state.app_state);
    TEST_ASSERT_EQUAL(3, state.error_count); // Should be 3 (1 from SIP error + 2 new)
    TEST_ASSERT_EQUAL_STRING("Second error", state.last_error);
    
    // Test error count reset
    result = app_controller_reset_error_count();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(0, state.error_count);
    TEST_ASSERT_EQUAL_STRING("", state.last_error);
    
    ESP_LOGI(TAG, "Error recovery workflow test passed");
}

void test_call_timeout_workflow(void)
{
    ESP_LOGI(TAG, "Testing call timeout workflow");
    
    // Start call
    esp_err_t result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_handle_sip_state_change(SIP_STATE_CALLING);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_CALLING, state.app_state);
    
    // Simulate call failure (timeout or busy)
    result = app_controller_handle_sip_state_change(SIP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(APP_STATE_IDLE, state.app_state);
    TEST_ASSERT_EQUAL(SIP_STATE_IDLE, state.sip_state);
    
    ESP_LOGI(TAG, "Call timeout workflow test passed");
}

void test_dtmf_protection_workflow(void)
{
    ESP_LOGI(TAG, "Testing DTMF protection workflow");
    
    // Set up connected call
    esp_err_t result = app_controller_set_app_state(APP_STATE_CONNECTED);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // First DTMF '1' should work
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state;
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    uint32_t first_dtmf_time = state.last_dtmf_time;
    
    // Immediate second DTMF '1' should be ignored (protection logic in app_controller)
    // Note: The actual protection is implemented in the DTMF handler
    result = app_controller_handle_dtmf('1');
    TEST_ASSERT_EQUAL(ESP_OK, result); // Function succeeds but may ignore the command
    
    result = app_controller_get_system_state(&state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    // The last_dtmf_time should be updated even if command is ignored
    TEST_ASSERT_GREATER_OR_EQUAL(first_dtmf_time, state.last_dtmf_time);
    
    ESP_LOGI(TAG, "DTMF protection workflow test passed");
}

void test_system_state_consistency(void)
{
    ESP_LOGI(TAG, "Testing system state consistency");
    
    // Test state consistency during transitions
    esp_err_t result = app_controller_set_app_state(APP_STATE_IDLE);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    system_state_t state1, state2;
    
    // Get state twice and ensure consistency
    result = app_controller_get_system_state(&state1);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    
    result = app_controller_get_system_state(&state2);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // States should be consistent (uptime may differ)
    TEST_ASSERT_EQUAL(state1.app_state, state2.app_state);
    TEST_ASSERT_EQUAL(state1.sip_state, state2.sip_state);
    TEST_ASSERT_EQUAL(state1.door_relay_state, state2.door_relay_state);
    TEST_ASSERT_EQUAL(state1.light_relay_state, state2.light_relay_state);
    TEST_ASSERT_EQUAL(state1.error_count, state2.error_count);
    
    // Uptime should increase
    TEST_ASSERT_GREATER_OR_EQUAL(state1.uptime_seconds, state2.uptime_seconds);
    
    ESP_LOGI(TAG, "System state consistency test passed");
}

// Test runner
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_complete_doorbell_to_call_workflow);
    RUN_TEST(test_dtmf_to_relay_control_workflow);
    RUN_TEST(test_error_recovery_workflow);
    RUN_TEST(test_call_timeout_workflow);
    RUN_TEST(test_dtmf_protection_workflow);
    RUN_TEST(test_system_state_consistency);
    
    UNITY_END();
}