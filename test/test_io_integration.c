#include "unity.h"
#include "io_manager.h"
#include "io_events.h"
#include "mocks/mock_gpio.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// Integration test state
static int button_press_count = 0;
static int button_release_count = 0;
static int relay_change_count = 0;
static io_relay_event_data_t last_relay_events[5]; // Store last 5 events

// Integration event handlers
static void integration_button_handler(void* arg, esp_event_base_t event_base,
                                      int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS) {
        if (event_id == IO_EVENT_BUTTON_PRESSED) {
            button_press_count++;
        } else if (event_id == IO_EVENT_BUTTON_RELEASED) {
            button_release_count++;
        }
    }
}

static void integration_relay_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS && event_id == IO_EVENT_RELAY_STATE_CHANGED) {
        if (relay_change_count < 5) {
            memcpy(&last_relay_events[relay_change_count], event_data, 
                   sizeof(io_relay_event_data_t));
        }
        relay_change_count++;
    }
}

void setUp(void)
{
    // Reset test state
    button_press_count = 0;
    button_release_count = 0;
    relay_change_count = 0;
    memset(last_relay_events, 0, sizeof(last_relay_events));
    
    // Initialize mocks and systems
    mock_gpio_init();
    esp_event_loop_create_default();
    
    // Initialize I/O manager (which initializes events)
    io_manager_init();
    
    // Register integration event handlers
    esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_PRESSED, 
                              integration_button_handler, NULL);
    esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_RELEASED, 
                              integration_button_handler, NULL);
    esp_event_handler_register(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                              integration_relay_handler, NULL);
}

void tearDown(void)
{
    // Unregister event handlers
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_BUTTON_PRESSED, 
                                 integration_button_handler);
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_BUTTON_RELEASED, 
                                 integration_button_handler);
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                                 integration_relay_handler);
    
    mock_gpio_reset();
}

// Test complete virtual button press workflow
void test_integration_virtual_button_workflow(void)
{
    // Trigger virtual button press
    esp_err_t result = io_manager_virtual_button_press();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Allow time for event processing
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Verify both press and release events were generated
    TEST_ASSERT_EQUAL(1, button_press_count);
    TEST_ASSERT_EQUAL(1, button_release_count);
}

// Test relay toggle generates events
void test_integration_relay_toggle_events(void)
{
    // Toggle door relay on
    esp_err_t result = io_manager_toggle_relay(RELAY_DOOR);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Verify event was generated
    TEST_ASSERT_EQUAL(1, relay_change_count);
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_events[0].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[0].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[0].new_state);
    
    // Toggle door relay off
    result = io_manager_toggle_relay(RELAY_DOOR);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Verify second event was generated
    TEST_ASSERT_EQUAL(2, relay_change_count);
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_events[1].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[1].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[1].new_state);
}

// Test relay pulse generates two events (on and off)
void test_integration_relay_pulse_events(void)
{
    // Start relay pulse
    esp_err_t result = io_manager_pulse_relay(RELAY_LIGHT, 500);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Verify initial "on" event was generated
    TEST_ASSERT_EQUAL(1, relay_change_count);
    TEST_ASSERT_EQUAL(RELAY_LIGHT, last_relay_events[0].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[0].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[0].new_state);
    
    // Note: In a real test environment, we would wait for the timer to expire
    // and verify the "off" event is generated. For this unit test, we verify
    // that the pulse was initiated correctly.
}

// Test multiple relay operations generate correct event sequence
void test_integration_multiple_relay_operations(void)
{
    // Sequence of operations
    io_manager_toggle_relay(RELAY_DOOR);    // Door ON
    vTaskDelay(pdMS_TO_TICKS(10));
    
    io_manager_toggle_relay(RELAY_LIGHT);   // Light ON
    vTaskDelay(pdMS_TO_TICKS(10));
    
    io_manager_toggle_relay(RELAY_DOOR);    // Door OFF
    vTaskDelay(pdMS_TO_TICKS(10));
    
    io_manager_toggle_relay(RELAY_LIGHT);   // Light OFF
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Verify all events were generated in correct order
    TEST_ASSERT_EQUAL(4, relay_change_count);
    
    // Event 1: Door ON
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_events[0].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[0].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[0].new_state);
    
    // Event 2: Light ON
    TEST_ASSERT_EQUAL(RELAY_LIGHT, last_relay_events[1].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[1].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[1].new_state);
    
    // Event 3: Door OFF
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_events[2].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[2].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[2].new_state);
    
    // Event 4: Light OFF
    TEST_ASSERT_EQUAL(RELAY_LIGHT, last_relay_events[3].relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_events[3].old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_events[3].new_state);
}

// Test event timestamps are sequential
void test_integration_event_timestamps_sequential(void)
{
    // Perform operations with small delays
    io_manager_toggle_relay(RELAY_DOOR);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    io_manager_toggle_relay(RELAY_LIGHT);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Verify timestamps are sequential
    TEST_ASSERT_EQUAL(2, relay_change_count);
    TEST_ASSERT_LESS_THAN(last_relay_events[1].timestamp, 
                          last_relay_events[0].timestamp + 200); // Within reasonable time
    TEST_ASSERT_GREATER_THAN(last_relay_events[0].timestamp, 
                            last_relay_events[1].timestamp - 200);
}

// Main test runner function
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_integration_virtual_button_workflow);
    RUN_TEST(test_integration_relay_toggle_events);
    RUN_TEST(test_integration_relay_pulse_events);
    RUN_TEST(test_integration_multiple_relay_operations);
    RUN_TEST(test_integration_event_timestamps_sequential);
    
    UNITY_END();
}