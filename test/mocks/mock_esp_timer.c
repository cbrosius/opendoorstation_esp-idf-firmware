#include "mock_esp_timer.h"
#include <string.h>

static mock_esp_timer_control_t s_timer_control;

void mock_esp_timer_init(void)
{
    mock_esp_timer_reset();
}

void mock_esp_timer_reset(void)
{
    memset(&s_timer_control, 0, sizeof(s_timer_control));
    s_timer_control.current_time_us = 0;
    s_timer_control.should_fail = false;
    s_timer_control.call_count = 0;
}

void mock_esp_timer_set_fail_mode(bool fail)
{
    s_timer_control.should_fail = fail;
}

void mock_esp_timer_advance_time(int64_t microseconds)
{
    s_timer_control.current_time_us += microseconds;
}

void mock_esp_timer_set_time(int64_t time_us)
{
    s_timer_control.current_time_us = time_us;
}

mock_esp_timer_control_t* mock_esp_timer_get_control(void)
{
    return &s_timer_control;
}

// Mock implementation of esp_timer_get_time
int64_t esp_timer_get_time(void)
{
    s_timer_control.call_count++;
    return s_timer_control.current_time_us;
}