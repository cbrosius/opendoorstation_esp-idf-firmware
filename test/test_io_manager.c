#include "unity.h"
#include "io_manager.h"
#include "mocks/mock_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

// Test fixture setup and teardown
void setUp(void)
{
    mock_gpio_init();
}

void tearDown(void)
{
    mock_gpio_reset();
}

// Test helper variables
static bool button_callback_called = false;
static bool button_callback_state = false;

static void test_button_callback(bool pressed)
{
    button_callback_called = true;
    button_callback_state = pressed;
}

// Test GPIO configuration during initialization
void test_io_manager_init_configures_gpio(void)
{
    esp_err_t result = io_manager_init();
    
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Check button GPIO configuration (GPIO 0)
    mock_gpio_state_t* button_state = mock_gpio_get_state(GPIO_NUM_0);
    TEST_ASSERT_NOT_NULL(button_state);
    TEST_ASSERT_EQUAL(GPIO_MODE_INPUT, button_state->mode);
    TEST_ASSERT_TRUE(button_state->pull_up);
    TEST_ASSERT_FALSE(button_state->pull_down);
    
    // Check door relay GPIO configuration (GPIO 2)
    mock_gpio_state_t* door_relay_state = mock_gpio_get_state(GPIO_NUM_2);
    TEST_ASSERT_NOT_NULL(door_relay_state);
    TEST_ASSERT_EQUAL(GPIO_MODE_OUTPUT, door_relay_state->mode);
    TEST_ASSERT_FALSE(door_relay_state->pull_up);
    TEST_ASSERT_FALSE(door_relay_state->pull_down);
    TEST_ASSERT_EQUAL(0, door_relay_state->level); // Should start OFF
    
    // Check light relay GPIO configuration (GPIO 3)
    mock_gpio_state_t* light_relay_state = mock_gpio_get_state(GPIO_NUM_3);
    TEST_ASSERT_NOT_NULL(light_relay_state);
    TEST_ASSERT_EQUAL(GPIO_MODE_OUTPUT, light_relay_state->mode);
    TEST_ASSERT_FALSE(light_relay_state->pull_up);
    TEST_ASSERT_FALSE(light_relay_state->pull_down);
    TEST_ASSERT_EQUAL(0, light_relay_state->level); // Should start OFF
}

// Test initial relay states
void test_io_manager_initial_relay_states(void)
{
    io_manager_init();
    
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_LIGHT));
}

// Test relay toggle functionality
void test_io_manager_toggle_relay(void)
{
    io_manager_init();
    
    // Test door relay toggle
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_DOOR));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_2));
    
    // Toggle again
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_DOOR));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(0, mock_gpio_get_output_level(GPIO_NUM_2));
    
    // Test light relay toggle
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_LIGHT));
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_3));
}

// Test relay pulse functionality
void test_io_manager_pulse_relay(void)
{
    io_manager_init();
    
    // Test valid pulse duration
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_pulse_relay(RELAY_DOOR, 1000));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_2));
    
    // Wait for pulse to complete (simulate timer callback)
    vTaskDelay(pdMS_TO_TICKS(1100));
    // Note: In real test, we would need to trigger the timer callback manually
    // For now, we test that the pulse was initiated correctly
}

// Test relay pulse parameter validation
void test_io_manager_pulse_relay_validation(void)
{
    io_manager_init();
    
    // Test invalid duration (too short)
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, io_manager_pulse_relay(RELAY_DOOR, 50));
    
    // Test invalid duration (too long)
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, io_manager_pulse_relay(RELAY_DOOR, 15000));
    
    // Test invalid relay ID
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, io_manager_pulse_relay(2, 1000));
}

// Test relay protection mechanism
void test_io_manager_relay_protection(void)
{
    io_manager_init();
    
    // First pulse should succeed
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_pulse_relay(RELAY_DOOR, 1000));
    
    // Second pulse within protection time should fail
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, io_manager_pulse_relay(RELAY_DOOR, 1000));
    
    // Note: In a real test, we would advance the mock timer to test that
    // the protection expires after 5 seconds
}

// Test button callback registration
void test_io_manager_button_callback_registration(void)
{
    io_manager_init();
    
    button_callback_called = false;
    
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_register_button_callback(test_button_callback));
    
    // Test virtual button press
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_virtual_button_press());
    
    // Note: In a real test environment, we would need to wait for the callback
    // to be called asynchronously. For this unit test, we verify the function
    // calls succeed without error.
}

// Test functions called before initialization
void test_io_manager_functions_before_init(void)
{
    // All functions should return ESP_ERR_INVALID_STATE before initialization
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, io_manager_pulse_relay(RELAY_DOOR, 1000));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, io_manager_toggle_relay(RELAY_DOOR));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, io_manager_register_button_callback(test_button_callback));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, io_manager_virtual_button_press());
    
    // Get relay state should return OFF for uninitialized state
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
}

// Test double initialization
void test_io_manager_double_init(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init()); // Should succeed but log warning
}

// Test invalid relay IDs
void test_io_manager_invalid_relay_ids(void)
{
    io_manager_init();
    
    // Test with invalid relay ID (>= 2)
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, io_manager_toggle_relay(2));
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, io_manager_pulse_relay(3, 1000));
    
    // Get state with invalid ID should return OFF
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(5));
}

// Main test runner function
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_io_manager_init_configures_gpio);
    RUN_TEST(test_io_manager_initial_relay_states);
    RUN_TEST(test_io_manager_toggle_relay);
    RUN_TEST(test_io_manager_pulse_relay);
    RUN_TEST(test_io_manager_pulse_relay_validation);
    RUN_TEST(test_io_manager_relay_protection);
    RUN_TEST(test_io_manager_button_callback_registration);
    RUN_TEST(test_io_manager_functions_before_init);
    RUN_TEST(test_io_manager_double_init);
    RUN_TEST(test_io_manager_invalid_relay_ids);
    
    UNITY_END();
}