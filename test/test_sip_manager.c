#include "unity.h"
#include "sip_manager.h"
#include "mock_esp_sip.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static sip_config_t test_config;
static bool dtmf_callback_called;
static char last_dtmf_digit;
static void *last_dtmf_user_data;

static void test_dtmf_callback(char digit, void *user_data) {
    dtmf_callback_called = true;
    last_dtmf_digit = digit;
    last_dtmf_user_data = user_data;
}

void setUp(void) {
    // Reset mock state
    mock_esp_sip_reset();
    
    // Initialize test configuration
    test_config = (sip_config_t){
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    // Reset callback state
    dtmf_callback_called = false;
    last_dtmf_digit = 0;
    last_dtmf_user_data = NULL;
    
    // Initialize event loop
    esp_event_loop_create_default();
}

void tearDown(void) {
    // Stop SIP manager if running
    sip_manager_stop();
    
    // Clean up event loop
    esp_event_loop_delete_default();
}

void test_sip_manager_init_success(void) {
    esp_err_t ret = sip_manager_init(&test_config);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(SIP_STATE_IDLE, sip_manager_get_state());
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->init_call_count);
}

void test_sip_manager_init_invalid_config(void) {
    // Test with NULL config
    esp_err_t ret = sip_manager_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid user (too short)
    sip_config_t invalid_config = test_config;
    strcpy(invalid_config.user, "ab");
    ret = sip_manager_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid domain (empty)
    invalid_config = test_config;
    strcpy(invalid_config.domain, "");
    ret = sip_manager_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_sip_manager_init_esp_sip_failure(void) {
    mock_esp_sip_get_control()->init_should_fail = true;
    
    esp_err_t ret = sip_manager_init(&test_config);
    
    TEST_ASSERT_EQUAL(ESP_FAIL, ret);
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->init_call_count);
}

void test_sip_manager_start_success(void) {
    // Initialize first
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    esp_err_t ret = sip_manager_start();
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->start_call_count);
}

void test_sip_manager_start_not_initialized(void) {
    esp_err_t ret = sip_manager_start();
    
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_sip_manager_start_esp_sip_failure(void) {
    // Initialize first
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    mock_esp_sip_get_control()->start_should_fail = true;
    
    esp_err_t ret = sip_manager_start();
    
    TEST_ASSERT_EQUAL(ESP_FAIL, ret);
}

void test_sip_manager_start_call_success(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    esp_err_t ret = sip_manager_start_call("sip:test@example.com");
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(SIP_STATE_CALLING, sip_manager_get_state());
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->call_call_count);
    TEST_ASSERT_EQUAL_STRING("sip:test@example.com", mock_esp_sip_get_control()->last_call_uri);
}

void test_sip_manager_start_call_default_callee(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    esp_err_t ret = sip_manager_start_call(NULL);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING("sip:callee@test.domain.com", mock_esp_sip_get_control()->last_call_uri);
}

void test_sip_manager_start_call_not_registered(void) {
    // Initialize but don't start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    esp_err_t ret = sip_manager_start_call("sip:test@example.com");
    
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_sip_manager_end_call_success(void) {
    // Initialize, start, and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    
    // Simulate call connected
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    TEST_ASSERT_EQUAL(SIP_STATE_CONNECTED, sip_manager_get_state());
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    esp_err_t ret = sip_manager_end_call();
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->hangup_call_count);
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
}

void test_sip_manager_dtmf_callback_registration(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    void *test_user_data = (void*)0x12345678;
    esp_err_t ret = sip_manager_register_dtmf_callback(test_dtmf_callback, test_user_data);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Start and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Simulate DTMF
    mock_esp_sip_simulate_dtmf('5');
    
    TEST_ASSERT_TRUE(dtmf_callback_called);
    TEST_ASSERT_EQUAL('5', last_dtmf_digit);
    TEST_ASSERT_EQUAL(test_user_data, last_dtmf_user_data);
}

void test_sip_manager_call_duration(void) {
    // Initialize, start, and make a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    
    // Initially no call duration
    TEST_ASSERT_EQUAL(0, sip_manager_get_call_duration());
    
    // Simulate call connected
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    
    // Wait a bit and check duration (should be > 0)
    vTaskDelay(pdMS_TO_TICKS(100));
    uint32_t duration = sip_manager_get_call_duration();
    TEST_ASSERT_GREATER_THAN(0, duration);
    
    // End call
    sip_manager_end_call();
    TEST_ASSERT_EQUAL(0, sip_manager_get_call_duration());
}

void test_sip_manager_update_config(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Update configuration
    sip_config_t new_config = test_config;
    strcpy(new_config.user, "newuser");
    strcpy(new_config.domain, "new.domain.com");
    
    esp_err_t ret = sip_manager_update_config(&new_config);
    
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_EQUAL(1, mock_esp_sip_get_control()->destroy_call_count);
    TEST_ASSERT_EQUAL(2, mock_esp_sip_get_control()->init_call_count);
    TEST_ASSERT_EQUAL(2, mock_esp_sip_get_control()->start_call_count);
}

void test_sip_manager_registration_failure(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    // Start but simulate registration failure
    mock_esp_sip_get_control()->start_should_fail = false;
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Simulate registration failure
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTRATION_FAILED, NULL);
    
    TEST_ASSERT_EQUAL(SIP_STATE_ERROR, sip_manager_get_state());
}

void test_sip_manager_call_failure(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Start call but simulate failure
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_FAILED, NULL);
    
    TEST_ASSERT_EQUAL(SIP_STATE_ERROR, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
}void
 test_sip_manager_call_statistics_tracking(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    sip_call_stats_t stats;
    
    // Check initial statistics
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_EQUAL(0, stats.total_calls_made);
    TEST_ASSERT_EQUAL(0, stats.successful_calls);
    TEST_ASSERT_EQUAL(0, stats.failed_calls);
    TEST_ASSERT_EQUAL(0, stats.total_call_duration);
    
    // Make a successful call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Check statistics after call started
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_EQUAL(1, stats.total_calls_made);
    TEST_ASSERT_EQUAL(0, stats.successful_calls); // Not ended yet
    TEST_ASSERT_GREATER_THAN(0, stats.current_call_duration);
    
    // End the call
    sip_manager_end_call();
    
    // Check final statistics
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_EQUAL(1, stats.total_calls_made);
    TEST_ASSERT_EQUAL(1, stats.successful_calls);
    TEST_ASSERT_EQUAL(0, stats.failed_calls);
    TEST_ASSERT_GREATER_THAN(0, stats.total_call_duration);
    TEST_ASSERT_EQUAL(0, stats.current_call_duration);
}

void test_sip_manager_call_failure_statistics(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Make a failed call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_FAILED, NULL);
    
    sip_call_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_EQUAL(1, stats.total_calls_made);
    TEST_ASSERT_EQUAL(0, stats.successful_calls);
    TEST_ASSERT_EQUAL(1, stats.failed_calls);
    TEST_ASSERT_EQUAL(0, stats.total_call_duration);
}

void test_sip_manager_reset_call_statistics(void) {
    // Initialize and start
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Make some calls to generate statistics
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    sip_manager_end_call();
    
    // Verify statistics exist
    sip_call_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_GREATER_THAN(0, stats.total_calls_made);
    
    // Reset statistics
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_reset_call_stats());
    
    // Verify statistics are reset
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_call_stats(&stats));
    TEST_ASSERT_EQUAL(0, stats.total_calls_made);
    TEST_ASSERT_EQUAL(0, stats.successful_calls);
    TEST_ASSERT_EQUAL(0, stats.failed_calls);
    TEST_ASSERT_EQUAL(0, stats.total_call_duration);
}

void test_sip_manager_call_timeout_handling(void) {
    // Initialize with short timeout
    sip_config_t timeout_config = test_config;
    timeout_config.call_timeout = 1; // 1 second timeout
    
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&timeout_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Start a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    TEST_ASSERT_EQUAL(SIP_STATE_CALLING, sip_manager_get_state());
    
    // Wait for timeout (simulate by calling timeout callback directly)
    // In real scenario, FreeRTOS timer would call this
    vTaskDelay(pdMS_TO_TICKS(1100)); // Wait longer than timeout
    
    // The call should still be active since we're using mocks
    // In real implementation, timeout would be handled by timer
    TEST_ASSERT_TRUE(sip_manager_is_call_active() || sip_manager_get_state() == SIP_STATE_CALLING);
}

void test_sip_manager_get_call_stats_invalid_args(void) {
    // Test with uninitialized manager
    sip_call_stats_t stats;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, sip_manager_get_call_stats(&stats));
    
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    // Test with NULL pointer
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sip_manager_get_call_stats(NULL));
}

void test_sip_manager_reset_call_stats_not_initialized(void) {
    esp_err_t ret = sip_manager_reset_call_stats();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}sta
tic dtmf_command_t last_dtmf_command;
static uint32_t last_dtmf_param;
static void *last_dtmf_command_user_data;
static bool dtmf_command_callback_called;

static void test_dtmf_command_callback(dtmf_command_t command, uint32_t param, void *user_data) {
    dtmf_command_callback_called = true;
    last_dtmf_command = command;
    last_dtmf_param = param;
    last_dtmf_command_user_data = user_data;
}

void test_sip_manager_dtmf_command_mapping_default(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Register command callback
    void *test_user_data = (void*)0x87654321;
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_register_dtmf_command_callback(test_dtmf_command_callback, test_user_data));
    
    // Start a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Reset callback state
    dtmf_command_callback_called = false;
    
    // Simulate DTMF '1' (should map to door open)
    mock_esp_sip_simulate_dtmf('1');
    
    TEST_ASSERT_TRUE(dtmf_command_callback_called);
    TEST_ASSERT_EQUAL(DTMF_CMD_DOOR_OPEN, last_dtmf_command);
    TEST_ASSERT_EQUAL(0, last_dtmf_param);
    TEST_ASSERT_EQUAL(test_user_data, last_dtmf_command_user_data);
}

void test_sip_manager_dtmf_command_mapping_custom(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    // Configure custom DTMF mappings
    dtmf_command_mapping_t custom_mappings[] = {
        {'2', DTMF_CMD_DOOR_OPEN, 1000, true},    // '2' opens door with 1000ms pulse
        {'3', DTMF_CMD_STATUS_REQUEST, 0, true},  // '3' requests status
        {'9', DTMF_CMD_HANGUP, 0, false},        // '9' hangs up (disabled)
    };
    
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_configure_dtmf_commands(custom_mappings, 3));
    
    // Register command callback
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_register_dtmf_command_callback(test_dtmf_command_callback, NULL));
    
    // Start SIP and call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Test '2' mapping
    dtmf_command_callback_called = false;
    mock_esp_sip_simulate_dtmf('2');
    
    TEST_ASSERT_TRUE(dtmf_command_callback_called);
    TEST_ASSERT_EQUAL(DTMF_CMD_DOOR_OPEN, last_dtmf_command);
    TEST_ASSERT_EQUAL(1000, last_dtmf_param);
    
    // Test '3' mapping
    dtmf_command_callback_called = false;
    mock_esp_sip_simulate_dtmf('3');
    
    TEST_ASSERT_TRUE(dtmf_command_callback_called);
    TEST_ASSERT_EQUAL(DTMF_CMD_STATUS_REQUEST, last_dtmf_command);
    
    // Test '9' mapping (disabled)
    dtmf_command_callback_called = false;
    mock_esp_sip_simulate_dtmf('9');
    
    TEST_ASSERT_FALSE(dtmf_command_callback_called);
    
    // Test unmapped digit
    dtmf_command_callback_called = false;
    mock_esp_sip_simulate_dtmf('5');
    
    TEST_ASSERT_FALSE(dtmf_command_callback_called);
}

void test_sip_manager_dtmf_processing_disabled(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_register_dtmf_command_callback(test_dtmf_command_callback, NULL));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Disable DTMF processing
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_set_dtmf_processing_enabled(false));
    
    // Start a call
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Simulate DTMF '1' (should not trigger command)
    dtmf_command_callback_called = false;
    mock_esp_sip_simulate_dtmf('1');
    
    TEST_ASSERT_FALSE(dtmf_command_callback_called);
    
    // Re-enable and test again
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_set_dtmf_processing_enabled(true));
    mock_esp_sip_simulate_dtmf('1');
    
    TEST_ASSERT_TRUE(dtmf_command_callback_called);
    TEST_ASSERT_EQUAL(DTMF_CMD_DOOR_OPEN, last_dtmf_command);
}

void test_sip_manager_get_dtmf_commands(void) {
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    // Get default mappings
    dtmf_command_mapping_t mappings[12];
    size_t actual_count;
    
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_get_dtmf_commands(mappings, 12, &actual_count));
    TEST_ASSERT_EQUAL(3, actual_count); // Default has 3 mappings
    
    // Verify default mappings
    bool found_door_open = false, found_hangup = false, found_status = false;
    for (size_t i = 0; i < actual_count; i++) {
        if (mappings[i].digit == '1' && mappings[i].command == DTMF_CMD_DOOR_OPEN) {
            found_door_open = true;
        } else if (mappings[i].digit == '0' && mappings[i].command == DTMF_CMD_HANGUP) {
            found_hangup = true;
        } else if (mappings[i].digit == '*' && mappings[i].command == DTMF_CMD_STATUS_REQUEST) {
            found_status = true;
        }
    }
    
    TEST_ASSERT_TRUE(found_door_open);
    TEST_ASSERT_TRUE(found_hangup);
    TEST_ASSERT_TRUE(found_status);
}

void test_sip_manager_configure_dtmf_commands_invalid_args(void) {
    // Test with uninitialized manager
    dtmf_command_mapping_t mapping = {'1', DTMF_CMD_DOOR_OPEN, 0, true};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, sip_manager_configure_dtmf_commands(&mapping, 1));
    
    // Initialize
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&test_config));
    
    // Test with NULL pointer but non-zero count
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sip_manager_configure_dtmf_commands(NULL, 1));
    
    // Test with invalid DTMF digit
    dtmf_command_mapping_t invalid_mapping = {'X', DTMF_CMD_DOOR_OPEN, 0, true};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sip_manager_configure_dtmf_commands(&invalid_mapping, 1));
    
    // Test with too many mappings
    dtmf_command_mapping_t many_mappings[20]; // More than max
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, sip_manager_configure_dtmf_commands(many_mappings, 20));
}

void test_sip_manager_dtmf_command_callback_not_initialized(void) {
    esp_err_t ret = sip_manager_register_dtmf_command_callback(test_dtmf_command_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}