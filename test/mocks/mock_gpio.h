#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#include "driver/gpio.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock GPIO state structure
 */
typedef struct {
    int level;          // Current GPIO level (0 or 1)
    gpio_mode_t mode;   // GPIO mode (input/output)
    bool pull_up;       // Pull-up enabled
    bool pull_down;     // Pull-down enabled
} mock_gpio_state_t;

/**
 * @brief Initialize GPIO mocking system
 */
void mock_gpio_init(void);

/**
 * @brief Reset all GPIO mock states
 */
void mock_gpio_reset(void);

/**
 * @brief Set mock GPIO level (simulates external input)
 * 
 * @param gpio_num GPIO number
 * @param level Level to set (0 or 1)
 */
void mock_gpio_set_input_level(gpio_num_t gpio_num, int level);

/**
 * @brief Get mock GPIO output level
 * 
 * @param gpio_num GPIO number
 * @return Current output level
 */
int mock_gpio_get_output_level(gpio_num_t gpio_num);

/**
 * @brief Get mock GPIO configuration
 * 
 * @param gpio_num GPIO number
 * @return Pointer to GPIO state structure
 */
mock_gpio_state_t* mock_gpio_get_state(gpio_num_t gpio_num);

/**
 * @brief Check if GPIO was configured
 * 
 * @param gpio_num GPIO number
 * @return true if configured, false otherwise
 */
bool mock_gpio_is_configured(gpio_num_t gpio_num);

#ifdef __cplusplus
}
#endif

#endif // MOCK_GPIO_H