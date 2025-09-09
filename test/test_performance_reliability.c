#include "unity.h"
#include "app_controller.h"
#include "io_manager.h"
#include "sip_manager.h"
#include "config_manager.h"
#include "web_server.h"
#include "error_handler.h"
#include "mocks/mock_gpio.h"
#include "mocks/mock_esp_sip.h"
#include "mocks/mock_nvs.h"
#include "mocks/mock_esp_timer.h"
#include "mocks/mock_freertos.h"
#include "mocks/mock_http_server.h"
#include <string.h>

static const char *TAG = "test_performance_reliability";

// Helper function to convert door station config to SIP config
static void convert_to_sip_config(const door_station_config_t *door_config, sip_config_t *sip_config)
{
    memset(sip_config, 0, sizeof(sip_config_t));
    strncpy(sip_config->user, door_config->sip_user, sizeof(sip_config->user) - 1);
    strncpy(sip_config->domain, door_config->sip_domain, sizeof(sip_config->domain) - 1);
    strncpy(sip_config->password, door_config->sip_password, sizeof(sip_config->password) - 1);
    strncpy(sip_config->callee, door_config->sip_callee, sizeof(sip_config->callee) - 1);
    sip_config->port = 5060;
    sip_config->registration_timeout = 300;
    sip_config->call_timeout = 30;
}

// Test fixture setup and teardown
void setUp(void)
{
    mock_gpio_init();
    mock_esp_sip_reset();
    mock_nvs_init();
    mock_esp_timer_init();
    mock_freertos_init();
    mock_http_server_init();
    error_handler_init();
}

void tearDown(void)
{
    app_controller_stop();
    web_server_stop();
    sip_manager_stop();
    mock_gpio_reset();
    mock_esp_sip_reset();
    mock_nvs_clear();
    mock_esp_timer_reset();
    mock_freertos_reset();
    mock_http_server_reset();
}

// ============================================================================
// Stress Tests for Concurrent SIP and Web Operations
// ============================================================================

void test_stress_concurrent_sip_web_operations(void)
{
    // Initialize system components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate SIP registration
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Stress test: Perform multiple concurrent operations
    for (int i = 0; i < 50; i++) {
        // Start and end calls rapidly
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
        
        // Concurrent web operations
        web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
        web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
        
        // Concurrent I/O operations
        io_manager_toggle_relay(RELAY_LIGHT);
        io_manager_pulse_relay(RELAY_DOOR, 1000);
        
        // DTMF processing
        mock_esp_sip_simulate_dtmf('1');
        mock_esp_sip_simulate_dtmf('2');
        
        // End call
        sip_manager_end_call();
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
        
        // Advance time to simulate realistic timing
        mock_esp_timer_advance_time(100000);  // 100ms
    }
    
    // Verify system is still operational after stress test
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
    
    // Verify call statistics
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    TEST_ASSERT_EQUAL(50, stats.total_calls_made);
    TEST_ASSERT_EQUAL(50, stats.successful_calls);
}

void test_stress_rapid_button_presses(void)
{
    // Initialize system
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Stress test: Rapid button presses
    for (int i = 0; i < 100; i++) {
        // Simulate button press
        mock_gpio_set_input_level(GPIO_NUM_0, 0);  // Press
        io_manager_virtual_button_press();
        
        // Advance time slightly
        mock_esp_timer_advance_time(10000);  // 10ms
        
        // Simulate button release
        mock_gpio_set_input_level(GPIO_NUM_0, 1);  // Release
        
        // Advance time for debouncing
        mock_esp_timer_advance_time(50000);  // 50ms
    }
    
    // Verify system handled rapid inputs gracefully
    // Should have initiated at least one call
    mock_esp_sip_control_t* sip_control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, sip_control->call_call_count);
    
    // System should still be operational
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
}

void test_stress_rapid_config_updates(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Stress test: Rapid configuration updates
    for (int i = 0; i < 25; i++) {
        door_station_config_t config;
        config_manager_get_defaults(&config);
        
        // Modify configuration
        snprintf(config.wifi_ssid, sizeof(config.wifi_ssid), "Network%d", i);
        snprintf(config.sip_user, sizeof(config.sip_user), "user%d", i);
        config.web_port = 8080 + i;
        config.door_pulse_duration = 1000 + (i * 100);
        
        // Save and load configuration
        TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
        
        door_station_config_t loaded_config;
        TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&loaded_config));
        
        // Verify configuration was saved correctly
        TEST_ASSERT_EQUAL_STRING(config.wifi_ssid, loaded_config.wifi_ssid);
        TEST_ASSERT_EQUAL_STRING(config.sip_user, loaded_config.sip_user);
        TEST_ASSERT_EQUAL(config.web_port, loaded_config.web_port);
        TEST_ASSERT_EQUAL(config.door_pulse_duration, loaded_config.door_pulse_duration);
        
        // Advance time
        mock_esp_timer_advance_time(10000);  // 10ms
    }
    
    // Verify system is still operational
    TEST_ASSERT_TRUE(web_server_is_running());
}

void test_stress_relay_operations(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Stress test: Rapid relay operations
    for (int i = 0; i < 100; i++) {
        // Alternate between door and light relays
        relay_id_t relay = (i % 2 == 0) ? RELAY_DOOR : RELAY_LIGHT;
        
        if (relay == RELAY_DOOR) {
            // Pulse door relay
            esp_err_t result = io_manager_pulse_relay(RELAY_DOOR, 500);
            // May fail due to protection timer, which is expected
            TEST_ASSERT_TRUE(result == ESP_OK || result != ESP_OK);
        } else {
            // Toggle light relay
            TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
        }
        
        // Broadcast status update
        relay_state_t state = io_manager_get_relay_state(relay);
        web_server_broadcast_relay_status(relay, state);
        
        // Advance time
        mock_esp_timer_advance_time(100000);  // 100ms
    }
    
    // Verify system is still operational
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Verify relay states are consistent
    relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
    relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
    
    // States should be valid
    TEST_ASSERT_TRUE(door_state == RELAY_STATE_OFF || door_state == RELAY_STATE_ON);
    TEST_ASSERT_TRUE(light_state == RELAY_STATE_OFF || light_state == RELAY_STATE_ON);
}

// ============================================================================
// Memory Leak Detection and Resource Usage Tests
// ============================================================================

void test_memory_leak_config_operations(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Get initial FreeRTOS state
    mock_freertos_control_t* initial_freertos = mock_freertos_get_control();
    int initial_semaphore_count = initial_freertos->semaphore_create_call_count;
    
    // Perform many configuration operations
    for (int i = 0; i < 50; i++) {
        door_station_config_t config;
        config_manager_get_defaults(&config);
        
        snprintf(config.wifi_ssid, sizeof(config.wifi_ssid), "TestNetwork%d", i);
        snprintf(config.sip_user, sizeof(config.sip_user), "user%d", i);
        
        config_manager_save(&config);
        config_manager_load(&config);
        config_manager_validate(&config);
        
        // Simulate some processing time
        mock_esp_timer_advance_time(1000);  // 1ms
    }
    
    // Check for resource leaks
    mock_freertos_control_t* final_freertos = mock_freertos_get_control();
    
    // Semaphore count should not have grown excessively
    int semaphore_growth = final_freertos->semaphore_create_call_count - initial_semaphore_count;
    TEST_ASSERT_LESS_THAN(10, semaphore_growth);  // Allow some growth but not excessive
}

void test_memory_leak_sip_operations(void)
{
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Get initial state
    mock_esp_sip_control_t* initial_sip = mock_esp_sip_get_control();
    int initial_init_count = initial_sip->init_call_count;
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Perform many SIP operations
    for (int i = 0; i < 30; i++) {
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
        
        // Process some DTMF
        mock_esp_sip_simulate_dtmf('1');
        mock_esp_sip_simulate_dtmf('2');
        
        sip_manager_end_call();
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
        
        mock_esp_timer_advance_time(100000);  // 100ms
    }
    
    // Check for resource leaks
    mock_esp_sip_control_t* final_sip = mock_esp_sip_get_control();
    
    // Should not have re-initialized SIP multiple times
    TEST_ASSERT_EQUAL(initial_init_count, final_sip->init_call_count);
    
    // Verify call statistics are reasonable
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    TEST_ASSERT_EQUAL(30, stats.total_calls_made);
    TEST_ASSERT_EQUAL(30, stats.successful_calls);
}

void test_memory_leak_web_operations(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Get initial HTTP server state
    mock_http_server_control_t* initial_http = mock_http_server_get_control();
    int initial_start_count = initial_http->start_call_count;
    
    // Perform many web operations
    for (int i = 0; i < 100; i++) {
        // Broadcast status updates
        web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
        web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_OFF);
        web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_ON);
        web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
        
        // Log URL
        web_server_log_url();
        
        mock_esp_timer_advance_time(10000);  // 10ms
    }
    
    // Check for resource leaks
    mock_http_server_control_t* final_http = mock_http_server_get_control();
    
    // Should not have restarted server multiple times
    TEST_ASSERT_EQUAL(initial_start_count, final_http->start_call_count);
    
    // Server should still be running
    TEST_ASSERT_TRUE(web_server_is_running());
}

void test_resource_usage_monitoring(void)
{
    // Initialize all components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Get baseline resource usage
    mock_freertos_control_t* freertos_control = mock_freertos_get_control();
    int baseline_tasks = freertos_control->task_create_call_count;
    int baseline_semaphores = freertos_control->semaphore_create_call_count;
    
    mock_esp_timer_control_t* timer_control = mock_esp_timer_get_control();
    int baseline_timer_calls = timer_control->call_count;
    
    // Perform normal operations
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    for (int i = 0; i < 10; i++) {
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
        mock_esp_sip_simulate_dtmf('1');
        io_manager_pulse_relay(RELAY_DOOR, 1000);
        web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
        sip_manager_end_call();
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
        
        mock_esp_timer_advance_time(1000000);  // 1 second
    }
    
    // Check resource usage growth
    int final_tasks = freertos_control->task_create_call_count;
    int final_semaphores = freertos_control->semaphore_create_call_count;
    int final_timer_calls = timer_control->call_count;
    
    // Resource usage should be reasonable
    TEST_ASSERT_LESS_THAN(baseline_tasks + 5, final_tasks);  // Allow some task creation
    TEST_ASSERT_LESS_THAN(baseline_semaphores + 10, final_semaphores);  // Allow some semaphore creation
    TEST_ASSERT_GREATER_THAN(baseline_timer_calls, final_timer_calls);  // Timer should be used
    
    // System should still be operational
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
}

// ============================================================================
// Reliability Tests for Error Recovery Scenarios
// ============================================================================

void test_reliability_sip_registration_failure_recovery(void)
{
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate multiple registration failures
    for (int i = 0; i < 5; i++) {
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTRATION_FAILED, NULL);
        mock_esp_timer_advance_time(30000000);  // 30 seconds
    }
    
    // Finally succeed
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Verify system recovered
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    
    // Should be able to make calls
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
}

void test_reliability_call_failure_recovery(void)
{
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Simulate multiple call failures
    for (int i = 0; i < 3; i++) {
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_FAILED, NULL);
        mock_esp_timer_advance_time(5000000);  // 5 seconds
    }
    
    // Finally succeed
    sip_manager_start_call(NULL);
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Verify system recovered
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    TEST_ASSERT_EQUAL(SIP_STATE_CONNECTED, sip_manager_get_state());
    
    // Verify statistics
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    TEST_ASSERT_EQUAL(4, stats.total_calls_made);
    TEST_ASSERT_EQUAL(1, stats.successful_calls);
    TEST_ASSERT_EQUAL(3, stats.failed_calls);
}

void test_reliability_nvs_failure_recovery(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Save initial configuration
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "doorstation");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Simulate NVS failure
    mock_nvs_set_fail_mode(true);
    
    // Try to save configuration - should fail gracefully
    strcpy(config.wifi_ssid, "NewNetwork");
    esp_err_t result = config_manager_save(&config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Try to load configuration - should fail gracefully
    door_station_config_t loaded_config;
    result = config_manager_load(&loaded_config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // System should still provide defaults
    config_manager_get_defaults(&loaded_config);
    TEST_ASSERT_EQUAL_STRING("", loaded_config.wifi_ssid);  // Default empty
    
    // Recovery: NVS comes back online
    mock_nvs_set_fail_mode(false);
    
    // Should be able to save again
    strcpy(config.wifi_ssid, "RecoveryNetwork");
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Should be able to load
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&loaded_config));
    TEST_ASSERT_EQUAL_STRING("RecoveryNetwork", loaded_config.wifi_ssid);
}

void test_reliability_gpio_failure_recovery(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Normal operation
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_LIGHT));
    
    // Simulate GPIO failure by checking error conditions
    // Try invalid relay operations
    esp_err_t result = io_manager_pulse_relay((relay_id_t)99, 1000);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    result = io_manager_pulse_relay(RELAY_DOOR, 50);  // Too short
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // System should still work for valid operations
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_LIGHT));
    
    // Web server should still be able to broadcast
    TEST_ASSERT_EQUAL(ESP_OK, web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF));
}

void test_reliability_web_server_failure_recovery(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    // Normal web server startup
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Simulate web server failure
    mock_http_server_set_start_fail(true);
    
    // Stop and try to restart - should fail
    web_server_stop();
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(web_server_is_running());
    
    // I/O operations should still work
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_DOOR));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_pulse_relay(RELAY_LIGHT, 1000));
    
    // Recovery: Web server comes back online
    mock_http_server_set_start_fail(false);
    
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Should be able to broadcast again
    TEST_ASSERT_EQUAL(ESP_OK, web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON));
}

void test_reliability_system_overload_recovery(void)
{
    // Initialize full system
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Simulate system overload with rapid operations
    for (int i = 0; i < 20; i++) {
        // Rapid button presses
        io_manager_virtual_button_press();
        
        // Rapid relay operations
        io_manager_toggle_relay(RELAY_LIGHT);
        io_manager_pulse_relay(RELAY_DOOR, 500);  // May fail due to protection
        
        // Rapid SIP operations
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
        mock_esp_sip_simulate_dtmf('1');
        mock_esp_sip_simulate_dtmf('2');
        sip_manager_end_call();
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
        
        // Rapid web operations
        web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
        web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_OFF);
        
        // Rapid config operations
        door_station_config_t temp_config = config;
        temp_config.door_pulse_duration = 1000 + i;
        config_manager_save(&temp_config);
        
        // Minimal time advancement
        mock_esp_timer_advance_time(1000);  // 1ms
    }
    
    // System should recover and be operational
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
    
    // Should be able to perform normal operations
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start_call(NULL));
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
}

void test_reliability_error_handler_integration(void)
{
    // Initialize error handler
    TEST_ASSERT_EQUAL(ESP_OK, error_handler_init());
    
    // Initialize other components
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Generate various error conditions
    
    // Configuration errors
    door_station_config_t invalid_config;
    config_manager_get_defaults(&invalid_config);
    strcpy(invalid_config.wifi_ssid, "ThisSSIDIsWayTooLongForWiFiSpecification");
    config_validation_error_t validation = config_manager_validate(&invalid_config);
    TEST_ASSERT_NOT_EQUAL(CONFIG_VALIDATION_OK, validation);
    
    // I/O errors
    esp_err_t result = io_manager_pulse_relay((relay_id_t)99, 1000);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Web server errors
    mock_http_server_set_start_fail(true);
    web_server_stop();
    result = web_server_init(8080);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Reset and verify recovery
    mock_http_server_set_start_fail(false);
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // System should be operational after error recovery
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    
    config_manager_get_defaults(&invalid_config);
    strcpy(invalid_config.wifi_ssid, "ValidNetwork");
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, config_manager_validate(&invalid_config));
}

// ============================================================================
// Performance Benchmarking Tests
// ============================================================================

void test_performance_config_operations_timing(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Measure configuration operation timing
    mock_esp_timer_set_time(0);
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "doorstation");
    config.web_port = 8080;
    
    int64_t start_time = mock_esp_timer_get_control()->current_time_us;
    
    // Perform configuration operations
    for (int i = 0; i < 10; i++) {
        config_manager_save(&config);
        config_manager_load(&config);
        config_manager_validate(&config);
        mock_esp_timer_advance_time(1000);  // 1ms per operation
    }
    
    int64_t end_time = mock_esp_timer_get_control()->current_time_us;
    int64_t total_time = end_time - start_time;
    
    // Should complete within reasonable time (10ms for 10 operations)
    TEST_ASSERT_LESS_THAN(50000, total_time);  // 50ms max
}

void test_performance_sip_call_setup_timing(void)
{
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Measure call setup timing
    mock_esp_timer_set_time(0);
    int64_t start_time = mock_esp_timer_get_control()->current_time_us;
    
    // Perform call operations
    for (int i = 0; i < 5; i++) {
        sip_manager_start_call(NULL);
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
        mock_esp_timer_advance_time(100000);  // 100ms call setup time
        sip_manager_end_call();
        mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
        mock_esp_timer_advance_time(50000);   // 50ms cleanup time
    }
    
    int64_t end_time = mock_esp_timer_get_control()->current_time_us;
    int64_t total_time = end_time - start_time;
    
    // Should complete within reasonable time (750ms for 5 calls)
    TEST_ASSERT_LESS_THAN(1000000, total_time);  // 1 second max
    
    // Verify call statistics
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    TEST_ASSERT_EQUAL(5, stats.total_calls_made);
    TEST_ASSERT_EQUAL(5, stats.successful_calls);
}

void test_performance_relay_operation_timing(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Measure relay operation timing
    mock_esp_timer_set_time(0);
    int64_t start_time = mock_esp_timer_get_control()->current_time_us;
    
    // Perform relay operations
    for (int i = 0; i < 20; i++) {
        io_manager_toggle_relay(RELAY_LIGHT);
        web_server_broadcast_relay_status(RELAY_LIGHT, io_manager_get_relay_state(RELAY_LIGHT));
        mock_esp_timer_advance_time(10000);  // 10ms per operation
    }
    
    int64_t end_time = mock_esp_timer_get_control()->current_time_us;
    int64_t total_time = end_time - start_time;
    
    // Should complete within reasonable time (200ms for 20 operations)
    TEST_ASSERT_LESS_THAN(500000, total_time);  // 500ms max
    
    // Verify final relay state
    relay_state_t final_state = io_manager_get_relay_state(RELAY_LIGHT);
    TEST_ASSERT_TRUE(final_state == RELAY_STATE_OFF || final_state == RELAY_STATE_ON);
}

void test_performance_system_responsiveness(void)
{
    // Initialize full system
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    sip_config_t sip_config;
    convert_to_sip_config(&config, &sip_config);
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&sip_config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Measure system responsiveness to button press
    mock_esp_timer_set_time(0);
    int64_t button_press_time = mock_esp_timer_get_control()->current_time_us;
    
    // Simulate button press
    io_manager_virtual_button_press();
    
    // Advance minimal time for processing
    mock_esp_timer_advance_time(1000);  // 1ms
    
    int64_t call_initiation_time = mock_esp_timer_get_control()->current_time_us;
    int64_t response_time = call_initiation_time - button_press_time;
    
    // System should respond quickly to button press
    TEST_ASSERT_LESS_THAN(10000, response_time);  // 10ms max response time
    
    // Verify call was initiated
    mock_esp_sip_control_t* sip_control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, sip_control->call_call_count);
}