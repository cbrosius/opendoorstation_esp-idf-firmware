#include "mock_gpio.h"
#include <string.h>

#define MAX_GPIO_NUM 48  // ESP32-S3 has GPIOs 0-47

static mock_gpio_state_t s_gpio_states[MAX_GPIO_NUM];
static bool s_gpio_configured[MAX_GPIO_NUM];

void mock_gpio_init(void)
{
    mock_gpio_reset();
}

void mock_gpio_reset(void)
{
    memset(s_gpio_states, 0, sizeof(s_gpio_states));
    memset(s_gpio_configured, 0, sizeof(s_gpio_configured));
    
    // Initialize all GPIOs to default state
    for (int i = 0; i < MAX_GPIO_NUM; i++) {
        s_gpio_states[i].level = 0;
        s_gpio_states[i].mode = GPIO_MODE_DISABLE;
        s_gpio_states[i].pull_up = false;
        s_gpio_states[i].pull_down = false;
    }
}

void mock_gpio_set_input_level(gpio_num_t gpio_num, int level)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        s_gpio_states[gpio_num].level = level ? 1 : 0;
    }
}

int mock_gpio_get_output_level(gpio_num_t gpio_num)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        return s_gpio_states[gpio_num].level;
    }
    return 0;
}

mock_gpio_state_t* mock_gpio_get_state(gpio_num_t gpio_num)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        return &s_gpio_states[gpio_num];
    }
    return NULL;
}

bool mock_gpio_is_configured(gpio_num_t gpio_num)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        return s_gpio_configured[gpio_num];
    }
    return false;
}

// Mock implementations of ESP-IDF GPIO functions
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig)
{
    if (!pGPIOConfig) {
        return ESP_ERR_INVALID_ARG;
    }

    // Configure each GPIO in the bit mask
    for (int i = 0; i < MAX_GPIO_NUM; i++) {
        if (pGPIOConfig->pin_bit_mask & (1ULL << i)) {
            s_gpio_states[i].mode = pGPIOConfig->mode;
            s_gpio_states[i].pull_up = (pGPIOConfig->pull_up_en == GPIO_PULLUP_ENABLE);
            s_gpio_states[i].pull_down = (pGPIOConfig->pull_down_en == GPIO_PULLDOWN_ENABLE);
            s_gpio_configured[i] = true;
            
            // Set default level for input pins with pull-up
            if (pGPIOConfig->mode == GPIO_MODE_INPUT && s_gpio_states[i].pull_up) {
                s_gpio_states[i].level = 1;
            }
        }
    }

    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        if (s_gpio_states[gpio_num].mode == GPIO_MODE_OUTPUT) {
            s_gpio_states[gpio_num].level = level ? 1 : 0;
            return ESP_OK;
        }
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_ERR_INVALID_ARG;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    if (gpio_num >= 0 && gpio_num < MAX_GPIO_NUM) {
        return s_gpio_states[gpio_num].level;
    }
    return 0;
}