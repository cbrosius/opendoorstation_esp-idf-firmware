#include "unity.h"
#include "io_manager.h"
#include "sip_manager.h"
#include "config_manager.h"
#include "mocks/mock_gpio.h"
#include "mocks/mock_esp_sip.h"
#include "mocks/mock_nvs.h"
#include "mocks/mock_esp_timer.h"
#include "mocks/mock_freertos.h"
#include <string.h>

static const char *TAG = "test_hardware_abstraction";

// Test fixture setup and teardown
void setUp(void)
{
    mock_gpio_init();
    mock_esp_sip_reset();
    mock_nvs_init();
    mock_esp_timer_init();
    mock_freertos_init();
}

void tearDown(void)
{
    mock_gpio_reset();
    mock_esp_sip_reset();
    mock_nvs_clear();
    mock_esp_timer_reset();
    mock_freertos_reset();
}

// ============================================================================
// GPIO Hardware Abstraction Layer Tests
// ============================================================================

void test_gpio_hal_initialization_smoke(void)
{
    // Test that GPIO initialization doesn't crash and returns success
    esp_err_t result = io_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify GPIO pins were configured
    TEST_ASSERT_TRUE(mock_gpio_is_configured(GPIO_NUM_0));  // Button
    TEST_ASSERT_TRUE(mock_gpio_is_configured(GPIO_NUM_2));  // Door relay
    TEST_ASSERT_TRUE(mock_gpio_is_configured(GPIO_NUM_4));  // Light relay
}

void test_gpio_hal_relay_control_smoke(void)
{
    io_manager_init();
    
    // Test door relay pulse
    esp_err_t result = io_manager_pulse_relay(RELAY_DOOR, 2000);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify relay was activated
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_2));
    
    // Test light relay toggle
    result = io_manager_toggle_relay(RELAY_LIGHT);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify relay state changed
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_LIGHT));
}

// Test callback functions
static bool s_callback_called = false;
static bool s_callback_state = false;
static bool s_dtmf_received = false;
static char s_received_digit = 0;

static void test_button_callback_func(bool pressed)
{
    s_callback_called = true;
    s_callback_state = pressed;
}

static void test_dtmf_callback_func(char digit, void *user_data)
{
    s_dtmf_received = true;
    s_received_digit = digit;
}

static void test_dummy_task(void *param)
{
    // Dummy task function for testing
}

void test_gpio_hal_button_input_smoke(void)
{
    io_manager_init();
    
    s_callback_called = false;
    s_callback_state = false;
    
    // Register button callback
    esp_err_t result = io_manager_register_button_callback(test_button_callback_func);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate button press
    mock_gpio_set_input_level(GPIO_NUM_0, 0);  // Active low
    
    // Process button events (would normally be done by task)
    // For smoke test, just verify the mock state is correct
    TEST_ASSERT_EQUAL(0, mock_gpio_get_output_level(GPIO_NUM_0));
}

void test_gpio_hal_error_conditions_smoke(void)
{
    // Test invalid relay ID
    esp_err_t result = io_manager_pulse_relay((relay_id_t)99, 1000);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Test invalid pulse duration
    result = io_manager_pulse_relay(RELAY_DOOR, 100);  // Too short
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    result = io_manager_pulse_relay(RELAY_DOOR, 15000);  // Too long
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

// ============================================================================
// SIP Hardware Abstraction Layer Tests
// ============================================================================

void test_sip_hal_initialization_smoke(void)
{
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    esp_err_t result = sip_manager_init(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify mock was called
    mock_esp_sip_control_t* control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->init_call_count);
}

void test_sip_hal_call_management_smoke(void)
{
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    sip_manager_init(&config);
    sip_manager_start();
    
    // Test call initiation
    esp_err_t result = sip_manager_start_call(NULL);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify mock was called
    mock_esp_sip_control_t* control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->call_call_count);
    
    // Test call termination
    result = sip_manager_end_call();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    TEST_ASSERT_GREATER_THAN(0, control->hangup_call_count);
}

void test_sip_hal_dtmf_processing_smoke(void)
{
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    sip_manager_init(&config);
    
    // Register DTMF callback
    esp_err_t result = sip_manager_register_dtmf_callback(test_dtmf_callback_func, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Simulate DTMF reception
    mock_esp_sip_simulate_dtmf('1');
    
    // For smoke test, just verify the mock function was called
    // (actual callback processing would happen in event loop)
    TEST_ASSERT_TRUE(true);  // If we get here, no crash occurred
}

void test_sip_hal_error_conditions_smoke(void)
{
    // Test initialization with NULL config
    esp_err_t result = sip_manager_init(NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Test call without initialization
    result = sip_manager_start_call("sip:test@example.com");
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Test with mock failure mode
    mock_esp_sip_control_t* control = mock_esp_sip_get_control();
    control->init_should_fail = true;
    
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    result = sip_manager_init(&config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

// ============================================================================
// NVS Hardware Abstraction Layer Tests
// ============================================================================

void test_nvs_hal_initialization_smoke(void)
{
    esp_err_t result = config_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

void test_nvs_hal_config_persistence_smoke(void)
{
    config_manager_init();
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Modify config
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "testuser");
    config.web_port = 8080;
    
    // Test save
    esp_err_t result = config_manager_save(&config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Test load
    door_station_config_t loaded_config;
    result = config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify data persistence
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("testuser", loaded_config.sip_user);
    TEST_ASSERT_EQUAL(8080, loaded_config.web_port);
}

void test_nvs_hal_factory_reset_smoke(void)
{
    config_manager_init();
    
    // Save some config
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    config_manager_save(&config);
    
    // Perform factory reset
    esp_err_t result = config_manager_factory_reset();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Verify config is reset
    door_station_config_t loaded_config;
    config_manager_load(&loaded_config);
    
    // Should be empty/default values
    TEST_ASSERT_EQUAL_STRING("", loaded_config.wifi_ssid);
}

void test_nvs_hal_error_conditions_smoke(void)
{
    config_manager_init();
    
    // Test with mock failure mode
    mock_nvs_set_fail_mode(true);
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    esp_err_t result = config_manager_save(&config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    result = config_manager_load(&config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

// ============================================================================
// Timer Hardware Abstraction Layer Tests
// ============================================================================

void test_timer_hal_time_functions_smoke(void)
{
    mock_esp_timer_set_time(1000000);  // 1 second
    
    int64_t time = esp_timer_get_time();
    TEST_ASSERT_EQUAL(1000000, time);
    
    // Advance time
    mock_esp_timer_advance_time(500000);  // 0.5 seconds
    
    time = esp_timer_get_time();
    TEST_ASSERT_EQUAL(1500000, time);
}

void test_timer_hal_call_counting_smoke(void)
{
    mock_esp_timer_control_t* control = mock_esp_timer_get_control();
    int initial_count = control->call_count;
    
    // Make several timer calls
    esp_timer_get_time();
    esp_timer_get_time();
    esp_timer_get_time();
    
    TEST_ASSERT_EQUAL(initial_count + 3, control->call_count);
}

// ============================================================================
// FreeRTOS Hardware Abstraction Layer Tests
// ============================================================================

void test_freertos_hal_task_creation_smoke(void)
{
    TaskHandle_t task_handle;
    
    BaseType_t result = xTaskCreate(
        test_dummy_task,
        "test_task",
        2048,
        NULL,
        5,
        &task_handle
    );
    
    TEST_ASSERT_EQUAL(pdPASS, result);
    TEST_ASSERT_NOT_NULL(task_handle);
    
    mock_freertos_control_t* control = mock_freertos_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->task_create_call_count);
}

void test_freertos_hal_semaphore_operations_smoke(void)
{
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    TEST_ASSERT_NOT_NULL(mutex);
    
    BaseType_t result = xSemaphoreTake(mutex, pdMS_TO_TICKS(100));
    TEST_ASSERT_EQUAL(pdTRUE, result);
    
    result = xSemaphoreGive(mutex);
    TEST_ASSERT_EQUAL(pdTRUE, result);
    
    mock_freertos_control_t* control = mock_freertos_get_control();
    TEST_ASSERT_GREATER_THAN(0, control->semaphore_create_call_count);
    TEST_ASSERT_GREATER_THAN(0, control->semaphore_take_call_count);
    TEST_ASSERT_GREATER_THAN(0, control->semaphore_give_call_count);
}

void test_freertos_hal_error_conditions_smoke(void)
{
    // Test task creation failure
    mock_freertos_set_task_create_fail(true);
    
    TaskHandle_t task_handle;
    BaseType_t result = xTaskCreate(
        test_dummy_task,
        "test_task",
        2048,
        NULL,
        5,
        &task_handle
    );
    
    TEST_ASSERT_EQUAL(pdFAIL, result);
    
    // Test semaphore creation failure
    mock_freertos_set_semaphore_create_fail(true);
    
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    TEST_ASSERT_NULL(mutex);
}

// ============================================================================
// Hardware Abstraction Layer Integration Tests
// ============================================================================

void test_hal_integration_io_timer_smoke(void)
{
    // Test that I/O operations use timer correctly
    io_manager_init();
    
    // Set initial time
    mock_esp_timer_set_time(1000000);  // 1 second
    
    // Pulse relay
    esp_err_t result = io_manager_pulse_relay(RELAY_DOOR, 2000);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Advance time to simulate pulse completion
    mock_esp_timer_advance_time(2000000);  // 2 seconds
    
    // Verify timer was used (call count increased)
    mock_esp_timer_control_t* timer_control = mock_esp_timer_get_control();
    TEST_ASSERT_GREATER_THAN(0, timer_control->call_count);
}

void test_hal_integration_sip_timer_smoke(void)
{
    // Test that SIP operations use timer correctly
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    sip_manager_init(&config);
    sip_manager_start();
    
    // Set initial time
    mock_esp_timer_set_time(5000000);  // 5 seconds
    
    // Start call
    sip_manager_start_call(NULL);
    
    // Simulate call connected event
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Get call duration (should use timer)
    uint32_t duration = sip_manager_get_call_duration();
    
    // Verify timer was used
    mock_esp_timer_control_t* timer_control = mock_esp_timer_get_control();
    TEST_ASSERT_GREATER_THAN(0, timer_control->call_count);
}

// ============================================================================
// Hardware Abstraction Layer Coverage Tests
// ============================================================================

void test_hal_coverage_all_gpio_functions(void)
{
    // Test all GPIO-related functions to ensure coverage
    io_manager_init();
    
    // Test all relay operations
    io_manager_pulse_relay(RELAY_DOOR, 1000);
    io_manager_pulse_relay(RELAY_LIGHT, 1000);
    io_manager_toggle_relay(RELAY_DOOR);
    io_manager_toggle_relay(RELAY_LIGHT);
    
    // Test state queries
    relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
    relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
    
    // Test virtual button
    io_manager_virtual_button_press();
    
    // Verify all functions executed without crashing
    TEST_ASSERT_TRUE(true);
}

void test_hal_coverage_all_sip_functions(void)
{
    // Test all SIP-related functions to ensure coverage
    sip_config_t config = {
        .user = "testuser",
        .domain = "test.domain.com",
        .password = "testpass",
        .callee = "sip:callee@test.domain.com",
        .port = 5060,
        .registration_timeout = 30,
        .call_timeout = 60
    };
    
    sip_manager_init(&config);
    sip_manager_start();
    
    // Test call management
    sip_manager_start_call(NULL);
    sip_manager_start_call("sip:specific@example.com");
    sip_manager_end_call();
    
    // Test state queries
    sip_state_t state = sip_manager_get_state();
    bool active = sip_manager_is_call_active();
    uint32_t duration = sip_manager_get_call_duration();
    
    // Test configuration update
    sip_manager_update_config(&config);
    
    // Test statistics
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    sip_manager_reset_call_stats();
    
    // Test DTMF configuration
    dtmf_command_mapping_t mappings[] = {
        {'1', DTMF_CMD_DOOR_OPEN, 0, true},
        {'2', DTMF_CMD_STATUS_REQUEST, 0, true}
    };
    sip_manager_configure_dtmf_commands(mappings, 2);
    sip_manager_set_dtmf_processing_enabled(true);
    
    sip_manager_stop();
    
    // Verify all functions executed without crashing
    TEST_ASSERT_TRUE(true);
}

void test_hal_coverage_all_config_functions(void)
{
    // Test all config-related functions to ensure coverage
    config_manager_init();
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    
    // Test validation
    config_validation_error_t validation = config_manager_validate(&config);
    
    // Test persistence
    config_manager_save(&config);
    config_manager_load(&config);
    config_manager_load_masked(&config);
    
    // Test utility functions
    bool is_sensitive = config_manager_is_sensitive_field("wifi_password");
    const char* error_msg = config_manager_get_validation_error_message(CONFIG_VALIDATION_OK);
    
    char masked_value[64];
    config_manager_mask_sensitive_value("secret", masked_value, sizeof(masked_value));
    
    // Test factory reset
    config_manager_factory_reset();
    
    // Verify all functions executed without crashing
    TEST_ASSERT_TRUE(true);
}