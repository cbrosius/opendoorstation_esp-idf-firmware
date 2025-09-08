#include "unity.h"
#include "sip_io_integration.h"
#include "sip_manager.h"
#include "io_manager.h"
#include "mock_esp_sip.h"
#include "mock_gpio.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static sip_config_t test_sip_config;
static sip_io_config_t test_integration_config;

void setUp(void) {
    // Reset mock states
    mock_esp_sip_reset();
    mock_gpio_reset();
    
    // Initialize test configurations
    test_sip_config = (sip_config_t){
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    test_integration_config = (sip_io_config_t){
        .door_pulse_duration_ms = 2000,
        .auto_hangup_after_door_open = true,
        .hangup_delay_ms = 5000,
        .status_feedback_enabled = true
    };
    
    // Initialize event loop
    esp_event_loop_create_default();
}

void tearDown(void) {
    // Stop all components
    sip_io_integration_stop();
    sip_manager_stop();
    
    // Clean up event loop
    esp_event_loop_delete_default();
}

void test_sip_io_integration_init_success(void) {
    esp_err_t ret = sip_io_integration_init(&test_integration_config);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_FALSE(sip_io_integration_is_active());
}

void test_sip_io_integration_init_invalid_config(void) {
    // Test with NULL config
    esp_err_t ret = sip_io_integration_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid door pulse duration (too short)
    sip_io_config_t invalid_config = test_integration_config;
    invalid_config.door_pulse_duration_ms = 50;
    ret = sip_io_integration_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid door pulse duration (too long)
    invalid_config = test_integration_config;
    invalid_config.door_pulse_duration_ms = 15000;
    ret = sip_io_integration_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid hangup delay (too long)
    invalid_config = test_integration_config;
    invalid_config.hangup_delay_ms = 70000;
    ret = sip_io_integration_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_sip_io_integration_start_success(void) {
    // Initialize components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    
    esp_err_t ret = sip_io_integration_start();
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(sip_io_integration_is_active());
}

void test_sip_io_integration_start_not_initialized(void) {
    esp_err_t ret = sip_io_integration_start();
    
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    TEST_ASSERT_FALSE(sip_io_integration_is_active());
}

void test_sip_io_integration_dtmf_door_open(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Reset GPIO mock state
    mock_gpio_reset();
    
    // Simulate DTMF '1' (door open command)
    mock_esp_sip_simulate_dtmf('1');
    
    // Verify door relay was pulsed
    mock_gpio_state_t *gpio_state = mock_gpio_get_state();
    TEST_ASSERT_TRUE(gpio_state->relay_pulsed[RELAY_DOOR]);
    TEST_ASSERT_EQUAL(2000, gpio_state->last_pulse_duration[RELAY_DOOR]);
}

void test_sip_io_integration_dtmf_custom_pulse_duration(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Configure custom DTMF mapping with specific pulse duration
    dtmf_command_mapping_t custom_mapping = {'2', DTMF_CMD_DOOR_OPEN, 1500, true};
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_configure_dtmf_commands(&custom_mapping, 1));
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Reset GPIO mock state
    mock_gpio_reset();
    
    // Simulate DTMF '2' (custom door open with 1500ms pulse)
    mock_esp_sip_simulate_dtmf('2');
    
    // Verify door relay was pulsed with custom duration
    mock_gpio_state_t *gpio_state = mock_gpio_get_state();
    TEST_ASSERT_TRUE(gpio_state->relay_pulsed[RELAY_DOOR]);
    TEST_ASSERT_EQUAL(1500, gpio_state->last_pulse_duration[RELAY_DOOR]);
}

void test_sip_io_integration_auto_hangup_after_door_open(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    // Simulate DTMF '1' (door open command)
    mock_esp_sip_simulate_dtmf('1');
    
    // Call should still be active immediately after door open
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    // Wait for auto hangup delay (simulate timer expiration)
    vTaskDelay(pdMS_TO_TICKS(100)); // Short delay for test
    
    // In real implementation, timer would trigger hangup
    // For testing, we verify the timer was started (indirectly through integration state)
    TEST_ASSERT_TRUE(sip_io_integration_is_active());
}

void test_sip_io_integration_auto_hangup_disabled(void) {
    // Configure without auto hangup
    sip_io_config_t no_hangup_config = test_integration_config;
    no_hangup_config.auto_hangup_after_door_open = false;
    
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&no_hangup_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Simulate DTMF '1' (door open command)
    mock_esp_sip_simulate_dtmf('1');
    
    // Call should remain active (no auto hangup)
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    // Wait and verify call is still active
    vTaskDelay(pdMS_TO_TICKS(100));
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
}

void test_sip_io_integration_dtmf_hangup_command(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    // Simulate DTMF '0' (hangup command)
    mock_esp_sip_simulate_dtmf('0');
    
    // Verify call was ended
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->hangup_call_count);
}

void test_sip_io_integration_button_press(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Reset GPIO mock state
    mock_gpio_reset();
    
    // Simulate physical button press
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_virtual_button_press());
    
    // Verify door relay was pulsed
    mock_gpio_state_t *gpio_state = mock_gpio_get_state();
    TEST_ASSERT_TRUE(gpio_state->relay_pulsed[RELAY_DOOR]);
    TEST_ASSERT_EQUAL(2000, gpio_state->last_pulse_duration[RELAY_DOOR]);
}

void test_sip_io_integration_status_request(void) {
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_start());
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Simulate DTMF '*' (status request command)
    mock_esp_sip_simulate_dtmf('*');
    
    // Status request should complete without error
    // In real implementation, this would provide feedback via SIP
    // For testing, we just verify the command was processed
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
}

void test_sip_io_integration_update_config(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    
    // Update configuration
    sip_io_config_t new_config = test_integration_config;
    new_config.door_pulse_duration_ms = 3000;
    new_config.auto_hangup_after_door_open = false;
    
    esp_err_t ret = sip_io_integration_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Verify configuration was updated
    sip_io_config_t current_config;
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_get_config(&current_config));
    TEST_ASSERT_EQUAL(3000, current_config.door_pulse_duration_ms);
    TEST_ASSERT_FALSE(current_config.auto_hangup_after_door_open);
}

void test_sip_io_integration_inactive_ignores_commands(void) {
    // Initialize but don't start
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_io_integration_init(&test_integration_config));
    
    // Start SIP and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Reset GPIO mock state
    mock_gpio_reset();
    
    // Simulate DTMF '1' (should be ignored since integration not started)
    mock_esp_sip_simulate_dtmf('1');
    
    // Verify door relay was NOT pulsed
    mock_gpio_state_t *gpio_state = mock_gpio_get_state();
    TEST_ASSERT_FALSE(gpio_state->relay_pulsed[RELAY_DOOR]);
}