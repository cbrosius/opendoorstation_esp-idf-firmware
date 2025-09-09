#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock FreeRTOS control structure
 */
typedef struct {
    uint32_t tick_count;
    bool task_create_should_fail;
    bool timer_create_should_fail;
    bool semaphore_create_should_fail;
    int task_create_call_count;
    int timer_create_call_count;
    int semaphore_create_call_count;
    int semaphore_take_call_count;
    int semaphore_give_call_count;
} mock_freertos_control_t;

/**
 * @brief Initialize mock FreeRTOS system
 */
void mock_freertos_init(void);

/**
 * @brief Reset mock FreeRTOS state
 */
void mock_freertos_reset(void);

/**
 * @brief Set mock FreeRTOS to simulate failures
 */
void mock_freertos_set_task_create_fail(bool fail);
void mock_freertos_set_timer_create_fail(bool fail);
void mock_freertos_set_semaphore_create_fail(bool fail);

/**
 * @brief Advance mock tick count
 */
void mock_freertos_advance_ticks(uint32_t ticks);

/**
 * @brief Get mock FreeRTOS control structure
 */
mock_freertos_control_t* mock_freertos_get_control(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_FREERTOS_H