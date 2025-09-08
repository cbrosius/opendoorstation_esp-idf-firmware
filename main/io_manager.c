#include "io_manager.h"
#include "io_events.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "io_manager";

// GPIO pin definitions
#define BUTTON_GPIO         GPIO_NUM_0      // Boot button on ESP32-S3
#define DOOR_RELAY_GPIO     GPIO_NUM_2      // Door relay control
#define LIGHT_RELAY_GPIO    GPIO_NUM_3      // Light relay control

// Timing constants
#define BUTTON_DEBOUNCE_MS  50              // Button debounce time
#define RELAY_PROTECTION_MS 5000            // Relay protection time (5 seconds)

// Internal state structure
typedef struct {
    bool initialized;
    relay_state_t relay_states[2];          // Door and light relay states
    button_callback_t button_callback;
    bool button_last_state;
    int64_t button_last_change_time;
    int64_t relay_last_pulse_time[2];       // Last pulse time for each relay
    TimerHandle_t relay_pulse_timers[2];    // Timers for relay pulse operations
} io_manager_state_t;

static io_manager_state_t s_io_state = {0};

// Forward declarations
static void button_task(void *arg);
static void relay_pulse_timer_callback(TimerHandle_t timer);
static esp_err_t configure_gpio_pins(void);

esp_err_t io_manager_init(void)
{
    if (s_io_state.initialized) {
        ESP_LOGW(TAG, "I/O manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing I/O manager...");

    // Initialize state
    memset(&s_io_state, 0, sizeof(s_io_state));
    s_io_state.relay_states[RELAY_DOOR] = RELAY_STATE_OFF;
    s_io_state.relay_states[RELAY_LIGHT] = RELAY_STATE_OFF;
    s_io_state.button_last_state = true; // Boot button is active low

    // Initialize event system
    esp_err_t ret = io_events_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I/O event system");
        return ret;
    }

    // Configure GPIO pins
    ret = configure_gpio_pins();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pins");
        return ret;
    }

    // Create relay pulse timers
    for (int i = 0; i < 2; i++) {
        s_io_state.relay_pulse_timers[i] = xTimerCreate(
            i == RELAY_DOOR ? "door_timer" : "light_timer",
            pdMS_TO_TICKS(1000), // Default 1 second, will be updated when used
            pdFALSE,             // One-shot timer
            (void *)(uintptr_t)i, // Timer ID is relay index
            relay_pulse_timer_callback
        );
        
        if (s_io_state.relay_pulse_timers[i] == NULL) {
            ESP_LOGE(TAG, "Failed to create relay timer %d", i);
            return ESP_ERR_NO_MEM;
        }
    }

    // Create button monitoring task
    BaseType_t task_ret = xTaskCreate(
        button_task,
        "button_task",
        2048,
        NULL,
        5,
        NULL
    );
    
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        return ESP_ERR_NO_MEM;
    }

    s_io_state.initialized = true;
    ESP_LOGI(TAG, "I/O manager initialized successfully");
    
    return ESP_OK;
}

esp_err_t io_manager_pulse_relay(relay_id_t relay, uint32_t duration_ms)
{
    if (!s_io_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (relay >= 2) {
        return ESP_ERR_INVALID_ARG;
    }

    if (duration_ms < 100 || duration_ms > 10000) {
        ESP_LOGE(TAG, "Invalid pulse duration: %lu ms (must be 100-10000ms)", duration_ms);
        return ESP_ERR_INVALID_ARG;
    }

    // Check relay protection (prevent multiple pulses within 5 seconds)
    int64_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
    if (current_time - s_io_state.relay_last_pulse_time[relay] < RELAY_PROTECTION_MS) {
        ESP_LOGW(TAG, "Relay %d pulse blocked by protection timer", relay);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Pulsing relay %d for %lu ms", relay, duration_ms);

    // Turn relay on
    gpio_num_t gpio_pin = (relay == RELAY_DOOR) ? DOOR_RELAY_GPIO : LIGHT_RELAY_GPIO;
    relay_state_t old_state = s_io_state.relay_states[relay];
    gpio_set_level(gpio_pin, 1);
    s_io_state.relay_states[relay] = RELAY_STATE_ON;
    s_io_state.relay_last_pulse_time[relay] = current_time;

    // Publish relay state change event
    io_events_publish_relay_state_change(relay, old_state, RELAY_STATE_ON);

    // Start timer to turn relay off
    xTimerChangePeriod(s_io_state.relay_pulse_timers[relay], pdMS_TO_TICKS(duration_ms), 0);
    xTimerStart(s_io_state.relay_pulse_timers[relay], 0);

    return ESP_OK;
}

esp_err_t io_manager_toggle_relay(relay_id_t relay)
{
    if (!s_io_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (relay >= 2) {
        return ESP_ERR_INVALID_ARG;
    }

    // Toggle relay state
    relay_state_t old_state = s_io_state.relay_states[relay];
    relay_state_t new_state = (old_state == RELAY_STATE_OFF) ? 
                              RELAY_STATE_ON : RELAY_STATE_OFF;
    
    gpio_num_t gpio_pin = (relay == RELAY_DOOR) ? DOOR_RELAY_GPIO : LIGHT_RELAY_GPIO;
    gpio_set_level(gpio_pin, new_state);
    s_io_state.relay_states[relay] = new_state;

    ESP_LOGI(TAG, "Relay %d toggled to %s", relay, 
             new_state == RELAY_STATE_ON ? "ON" : "OFF");

    // Publish relay state change event
    io_events_publish_relay_state_change(relay, old_state, new_state);

    return ESP_OK;
}

relay_state_t io_manager_get_relay_state(relay_id_t relay)
{
    if (!s_io_state.initialized || relay >= 2) {
        return RELAY_STATE_OFF;
    }

    return s_io_state.relay_states[relay];
}

esp_err_t io_manager_register_button_callback(button_callback_t callback)
{
    if (!s_io_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_io_state.button_callback = callback;
    ESP_LOGI(TAG, "Button callback registered");
    
    return ESP_OK;
}

esp_err_t io_manager_virtual_button_press(void)
{
    if (!s_io_state.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Virtual button press triggered");
    
    // Publish button press event
    io_events_publish_button(true);
    
    if (s_io_state.button_callback) {
        s_io_state.button_callback(true);
    }
    
    // Simulate button release after short delay
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Publish button release event
    io_events_publish_button(false);
    
    if (s_io_state.button_callback) {
        s_io_state.button_callback(false);
    }

    return ESP_OK;
}

// Private functions

static esp_err_t configure_gpio_pins(void)
{
    // Configure button GPIO (input with pull-up)
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&button_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO");
        return ret;
    }

    // Configure relay GPIOs (outputs)
    gpio_config_t relay_config = {
        .pin_bit_mask = (1ULL << DOOR_RELAY_GPIO) | (1ULL << LIGHT_RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    ret = gpio_config(&relay_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure relay GPIOs");
        return ret;
    }

    // Initialize relay outputs to OFF
    gpio_set_level(DOOR_RELAY_GPIO, 0);
    gpio_set_level(LIGHT_RELAY_GPIO, 0);

    ESP_LOGI(TAG, "GPIO pins configured - Button: %d, Door Relay: %d, Light Relay: %d",
             BUTTON_GPIO, DOOR_RELAY_GPIO, LIGHT_RELAY_GPIO);

    return ESP_OK;
}

static void button_task(void *arg)
{
    ESP_LOGI(TAG, "Button monitoring task started");

    while (1) {
        // Read button state (active low)
        bool current_state = !gpio_get_level(BUTTON_GPIO);
        int64_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds

        // Check if state changed and debounce time has passed
        if (current_state != s_io_state.button_last_state) {
            if (current_time - s_io_state.button_last_change_time >= BUTTON_DEBOUNCE_MS) {
                s_io_state.button_last_state = current_state;
                s_io_state.button_last_change_time = current_time;

                ESP_LOGI(TAG, "Button %s", current_state ? "PRESSED" : "RELEASED");

                // Publish button event
                io_events_publish_button(current_state);

                // Call registered callback
                if (s_io_state.button_callback) {
                    s_io_state.button_callback(current_state);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Check every 10ms
    }
}

static void relay_pulse_timer_callback(TimerHandle_t timer)
{
    // Get relay index from timer ID
    relay_id_t relay = (relay_id_t)(uintptr_t)pvTimerGetTimerID(timer);
    
    if (relay < 2) {
        // Turn relay off
        gpio_num_t gpio_pin = (relay == RELAY_DOOR) ? DOOR_RELAY_GPIO : LIGHT_RELAY_GPIO;
        relay_state_t old_state = s_io_state.relay_states[relay];
        gpio_set_level(gpio_pin, 0);
        s_io_state.relay_states[relay] = RELAY_STATE_OFF;
        
        ESP_LOGI(TAG, "Relay %d pulse completed, turned OFF", relay);

        // Publish relay state change event
        io_events_publish_relay_state_change(relay, old_state, RELAY_STATE_OFF);
    }
}