#ifndef MOCK_ESP_TIMER_H
#define MOCK_ESP_TIMER_H

#include "esp_timer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock timer control structure
 */
typedef struct {
    int64_t current_time_us;
    bool should_fail;
    int call_count;
} mock_esp_timer_control_t;

/**
 * @brief Initialize mock timer system
 */
void mock_esp_timer_init(void);

/**
 * @brief Reset mock timer state
 */
void mock_esp_timer_reset(void);

/**
 * @brief Set mock timer to simulate failure
 */
void mock_esp_timer_set_fail_mode(bool fail);

/**
 * @brief Advance mock timer by specified microseconds
 */
void mock_esp_timer_advance_time(int64_t microseconds);

/**
 * @brief Set absolute mock timer time
 */
void mock_esp_timer_set_time(int64_t time_us);

/**
 * @brief Get mock timer control structure
 */
mock_esp_timer_control_t* mock_esp_timer_get_control(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_TIMER_H