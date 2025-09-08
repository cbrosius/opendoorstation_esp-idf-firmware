#include "unity.h"
#include "io_events.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// Test event handler state
static bool button_event_received = false;
static bool relay_event_received = false;
static io_button_event_data_t last_button_event;
static io_relay_event_data_t last_relay_event;

// Test event handlers
static void test_button_event_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS) {
        if (event_id == IO_EVENT_BUTTON_PRESSED || event_id == IO_EVENT_BUTTON_RELEASED) {
            button_event_received = true;
            memcpy(&last_button_event, event_data, sizeof(io_button_event_data_t));
        }
    }
}

static void test_relay_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_base == IO_EVENTS && event_id == IO_EVENT_RELAY_STATE_CHANGED) {
        relay_event_received = true;
        memcpy(&last_relay_event, event_data, sizeof(io_relay_event_data_t));
    }
}

void setUp(void)
{
    // Reset test state
    button_event_received = false;
    relay_event_received = false;
    memset(&last_button_event, 0, sizeof(last_button_event));
    memset(&last_relay_event, 0, sizeof(last_relay_event));
    
    // Create default event loop if it doesn't exist
    esp_event_loop_create_default();
    
    // Initialize I/O events
    io_events_init();
    
    // Register test event handlers
    esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_PRESSED, 
                              test_button_event_handler, NULL);
    esp_event_handler_register(IO_EVENTS, IO_EVENT_BUTTON_RELEASED, 
                              test_button_event_handler, NULL);
    esp_event_handler_register(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                              test_relay_event_handler, NULL);
}

void tearDown(void)
{
    // Unregister event handlers
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_BUTTON_PRESSED, 
                                 test_button_event_handler);
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_BUTTON_RELEASED, 
                                 test_button_event_handler);
    esp_event_handler_unregister(IO_EVENTS, IO_EVENT_RELAY_STATE_CHANGED, 
                                 test_relay_event_handler);
}

// Test I/O events initialization
void test_io_events_init(void)
{
    esp_err_t result = io_events_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

// Test button press event publishing
void test_io_events_publish_button_press(void)
{
    esp_err_t result = io_events_publish_button(true);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Allow time for event processing
    vTaskDelay(pdMS_TO_TICKS(10));
    
    TEST_ASSERT_TRUE(button_event_received);
    TEST_ASSERT_TRUE(last_button_event.pressed);
    TEST_ASSERT_NOT_EQUAL(0, last_button_event.timestamp);
}

// Test button release event publishing
void test_io_events_publish_button_release(void)
{
    esp_err_t result = io_events_publish_button(false);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Allow time for event processing
    vTaskDelay(pdMS_TO_TICKS(10));
    
    TEST_ASSERT_TRUE(button_event_received);
    TEST_ASSERT_FALSE(last_button_event.pressed);
    TEST_ASSERT_NOT_EQUAL(0, last_button_event.timestamp);
}

// Test relay state change event publishing
void test_io_events_publish_relay_state_change(void)
{
    esp_err_t result = io_events_publish_relay_state_change(RELAY_DOOR, 
                                                           RELAY_STATE_OFF, 
                                                           RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    
    // Allow time for event processing
    vTaskDelay(pdMS_TO_TICKS(10));
    
    TEST_ASSERT_TRUE(relay_event_received);
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_event.relay);
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, last_relay_event.old_state);
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, last_relay_event.new_state);
    TEST_ASSERT_NOT_EQUAL(0, last_relay_event.timestamp);
}

// Test multiple relay events
void test_io_events_multiple_relay_events(void)
{
    // First event - door relay on
    esp_err_t result = io_events_publish_relay_state_change(RELAY_DOOR, 
                                                           RELAY_STATE_OFF, 
                                                           RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    TEST_ASSERT_TRUE(relay_event_received);
    TEST_ASSERT_EQUAL(RELAY_DOOR, last_relay_event.relay);
    
    // Reset for next event
    relay_event_received = false;
    
    // Second event - light relay on
    result = io_events_publish_relay_state_change(RELAY_LIGHT, 
                                                 RELAY_STATE_OFF, 
                                                 RELAY_STATE_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    TEST_ASSERT_TRUE(relay_event_received);
    TEST_ASSERT_EQUAL(RELAY_LIGHT, last_relay_event.relay);
}

// Test event timestamp accuracy
void test_io_events_timestamp_accuracy(void)
{
    uint32_t before_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    esp_err_t result = io_events_publish_button(true);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    uint32_t after_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    TEST_ASSERT_TRUE(button_event_received);
    TEST_ASSERT_GREATER_OR_EQUAL(before_time, last_button_event.timestamp);
    TEST_ASSERT_LESS_OR_EQUAL(after_time, last_button_event.timestamp);
}

// Main test runner function
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_io_events_init);
    RUN_TEST(test_io_events_publish_button_press);
    RUN_TEST(test_io_events_publish_button_release);
    RUN_TEST(test_io_events_publish_relay_state_change);
    RUN_TEST(test_io_events_multiple_relay_events);
    RUN_TEST(test_io_events_timestamp_accuracy);
    
    UNITY_END();
}