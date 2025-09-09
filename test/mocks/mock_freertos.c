#include "mock_freertos.h"
#include <string.h>

static mock_freertos_control_t s_freertos_control;
static SemaphoreHandle_t s_mock_semaphores[10];
static int s_semaphore_count = 0;

void mock_freertos_init(void)
{
    mock_freertos_reset();
}

void mock_freertos_reset(void)
{
    memset(&s_freertos_control, 0, sizeof(s_freertos_control));
    s_freertos_control.tick_count = 0;
    s_freertos_control.task_create_should_fail = false;
    s_freertos_control.timer_create_should_fail = false;
    s_freertos_control.semaphore_create_should_fail = false;
    s_semaphore_count = 0;
    memset(s_mock_semaphores, 0, sizeof(s_mock_semaphores));
}

void mock_freertos_set_task_create_fail(bool fail)
{
    s_freertos_control.task_create_should_fail = fail;
}

void mock_freertos_set_timer_create_fail(bool fail)
{
    s_freertos_control.timer_create_should_fail = fail;
}

void mock_freertos_set_semaphore_create_fail(bool fail)
{
    s_freertos_control.semaphore_create_should_fail = fail;
}

void mock_freertos_advance_ticks(uint32_t ticks)
{
    s_freertos_control.tick_count += ticks;
}

mock_freertos_control_t* mock_freertos_get_control(void)
{
    return &s_freertos_control;
}

// Mock implementations of FreeRTOS functions
TickType_t xTaskGetTickCount(void)
{
    return s_freertos_control.tick_count;
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, 
                      const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters,
                      UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask)
{
    s_freertos_control.task_create_call_count++;
    
    if (s_freertos_control.task_create_should_fail) {
        return pdFAIL;
    }
    
    // Return a mock task handle
    if (pxCreatedTask) {
        *pxCreatedTask = (TaskHandle_t)0x12345678;
    }
    
    return pdPASS;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    s_freertos_control.semaphore_create_call_count++;
    
    if (s_freertos_control.semaphore_create_should_fail) {
        return NULL;
    }
    
    if (s_semaphore_count < 10) {
        s_mock_semaphores[s_semaphore_count] = (SemaphoreHandle_t)(0x10000000 + s_semaphore_count);
        return s_mock_semaphores[s_semaphore_count++];
    }
    
    return NULL;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    s_freertos_control.semaphore_take_call_count++;
    
    // Always succeed for mock
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    s_freertos_control.semaphore_give_call_count++;
    
    // Always succeed for mock
    return pdTRUE;
}

void vTaskDelay(const TickType_t xTicksToDelay)
{
    // Advance tick count to simulate delay
    s_freertos_control.tick_count += xTicksToDelay;
}