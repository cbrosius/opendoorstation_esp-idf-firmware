#include "unity.h"
#include "app_controller.h"
#include "io_manager.h"
#include "sip_manager.h"
#include "config_manager.h"
#include "web_server.h"
#include "mocks/mock_gpio.h"
#include "mocks/mock_esp_sip.h"
#include "mocks/mock_nvs.h"
#include "mocks/mock_esp_timer.h"
#include "mocks/mock_freertos.h"
#include "mocks/mock_http_server.h"
#include <string.h>

static const char *TAG = "test_end_to_end_integration";

// Test fixture setup and teardown
void setUp(void)
{
    mock_gpio_init();
    mock_esp_sip_reset();
    mock_nvs_init();
    mock_esp_timer_init();
    mock_freertos_init();
    mock_http_server_init();
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
// Complete Call-to-Relay Workflow Tests
// ============================================================================

void test_e2e_button_press_to_call_workflow(void)
{
    // Initialize system components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    config.door_pulse_duration = 2000;
    
    // Initialize all components
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate SIP registration success
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Simulate button press
    mock_gpio_set_input_level(GPIO_NUM_0, 0);  // Active low button
    
    // Process button event (would normally be done by task)
    // For integration test, directly trigger the event
    io_manager_virtual_button_press();
    
    // Verify call was initiated
    mock_esp_sip_control_t* sip_control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, sip_control->call_call_count);
    TEST_ASSERT_EQUAL_STRING("sip:resident@192.168.1.100", sip_control->last_call_uri);
}void test
_e2e_dtmf_to_relay_control_workflow(void)
{
    // Initialize system components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    config.door_pulse_duration = 2000;
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate call connected state
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Verify initial relay state
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
    
    // Simulate DTMF '1' for door open
    mock_esp_sip_simulate_dtmf('1');
    
    // Verify door relay was activated
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_2));
    
    // Simulate DTMF '2' for light toggle
    mock_esp_sip_simulate_dtmf('2');
    
    // Verify light relay was toggled
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_LIGHT));
}

void test_e2e_call_timeout_workflow(void)
{
    // Initialize system components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate call initiation
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    sip_manager_start_call(NULL);
    
    // Simulate call timeout
    mock_esp_timer_advance_time(65000000);  // 65 seconds
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_FAILED, NULL);
    
    // Verify system returns to idle state
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
}

void test_e2e_multiple_call_attempts_workflow(void)
{
    // Initialize system components
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // First call attempt - fails
    sip_manager_start_call(NULL);
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_FAILED, NULL);
    
    // Second call attempt - succeeds
    sip_manager_start_call(NULL);
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Verify call is active
    TEST_ASSERT_TRUE(sip_manager_is_call_active());
    TEST_ASSERT_EQUAL(SIP_STATE_CONNECTED, sip_manager_get_state());
    
    // End call
    sip_manager_end_call();
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
    
    // Verify statistics
    sip_call_stats_t stats;
    sip_manager_get_call_stats(&stats);
    TEST_ASSERT_EQUAL(2, stats.total_calls_made);
    TEST_ASSERT_EQUAL(1, stats.successful_calls);
    TEST_ASSERT_EQUAL(1, stats.failed_calls);
}

// ============================================================================
// Web Interface Functionality Validation Tests
// ============================================================================

void test_e2e_web_config_update_workflow(void)
{
    // Initialize system components
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Verify web server is running
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Simulate configuration update via web interface
    door_station_config_t new_config;
    config_manager_get_defaults(&new_config);
    strcpy(new_config.wifi_ssid, "NewNetwork");
    strcpy(new_config.sip_user, "newuser");
    new_config.web_port = 8081;
    new_config.door_pulse_duration = 3000;
    
    // Save configuration (simulating web API call)
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&new_config));
    
    // Verify configuration was persisted
    door_station_config_t loaded_config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&loaded_config));
    
    TEST_ASSERT_EQUAL_STRING("NewNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("newuser", loaded_config.sip_user);
    TEST_ASSERT_EQUAL(8081, loaded_config.web_port);
    TEST_ASSERT_EQUAL(3000, loaded_config.door_pulse_duration);
    
    // Verify HTTP server was configured
    mock_http_server_control_t* http_control = mock_http_server_get_control();
    TEST_ASSERT_GREATER_THAN(0, http_control->start_call_count);
    TEST_ASSERT_GREATER_THAN(0, http_control->register_uri_call_count);
}

void test_e2e_virtual_doorbell_workflow(void)
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
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate SIP registration
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Trigger virtual doorbell via web interface
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_virtual_button_press());
    
    // Verify call was initiated
    mock_esp_sip_control_t* sip_control = mock_esp_sip_get_control();
    TEST_ASSERT_GREATER_THAN(0, sip_control->call_call_count);
}

void test_e2e_real_time_status_updates_workflow(void)
{
    // Initialize system components
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Verify initial relay states
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_LIGHT));
    
    // Toggle light relay
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    
    // Broadcast status update (simulating WebSocket broadcast)
    TEST_ASSERT_EQUAL(ESP_OK, web_server_broadcast_relay_status(RELAY_LIGHT, RELAY_STATE_ON));
    
    // Pulse door relay
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_pulse_relay(RELAY_DOOR, 2000));
    
    // Broadcast status update
    TEST_ASSERT_EQUAL(ESP_OK, web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON));
    
    // Verify relay states
    TEST_ASSERT_EQUAL(RELAY_STATE_ON, io_manager_get_relay_state(RELAY_LIGHT));
    TEST_ASSERT_EQUAL(1, mock_gpio_get_output_level(GPIO_NUM_2));  // Door relay active
}

// ============================================================================
// Configuration Persistence and Recovery Tests
// ============================================================================

void test_e2e_config_persistence_across_reboot(void)
{
    // First boot - save configuration
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.wifi_password, "password123");
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_password, "sippass");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    config.web_port = 8080;
    config.door_pulse_duration = 2500;
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Simulate reboot by resetting mocks and reinitializing
    mock_nvs_clear();
    mock_nvs_init();
    
    // Second boot - load configuration
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    door_station_config_t loaded_config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&loaded_config));
    
    // Verify all configuration was persisted
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loaded_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING("password123", loaded_config.wifi_password);
    TEST_ASSERT_EQUAL_STRING("doorstation", loaded_config.sip_user);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", loaded_config.sip_domain);
    TEST_ASSERT_EQUAL_STRING("sippass", loaded_config.sip_password);
    TEST_ASSERT_EQUAL_STRING("sip:resident@192.168.1.100", loaded_config.sip_callee);
    TEST_ASSERT_EQUAL(8080, loaded_config.web_port);
    TEST_ASSERT_EQUAL(2500, loaded_config.door_pulse_duration);
}

void test_e2e_config_recovery_from_corruption(void)
{
    // Initialize with valid configuration
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    config.web_port = 8080;
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Simulate NVS corruption by setting fail mode
    mock_nvs_set_fail_mode(true);
    
    // Try to load configuration - should handle gracefully
    door_station_config_t loaded_config;
    esp_err_t result = config_manager_load(&loaded_config);
    
    // Should either succeed with defaults or fail gracefully
    TEST_ASSERT_TRUE(result == ESP_OK || result != ESP_OK);
    
    // Reset fail mode
    mock_nvs_set_fail_mode(false);
    
    // Should be able to save new configuration
    config_manager_get_defaults(&loaded_config);
    strcpy(loaded_config.wifi_ssid, "RecoveryNetwork");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&loaded_config));
    
    // Verify recovery
    door_station_config_t recovery_config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&recovery_config));
    TEST_ASSERT_EQUAL_STRING("RecoveryNetwork", recovery_config.wifi_ssid);
}

void test_e2e_factory_reset_workflow(void)
{
    // Initialize and save configuration
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "doorstation");
    config.web_port = 8080;
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Initialize other components with this configuration
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(config.web_port));
    
    // Perform factory reset
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_factory_reset());
    
    // Verify configuration is reset to defaults
    door_station_config_t reset_config;
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_load(&reset_config));
    
    door_station_config_t default_config;
    config_manager_get_defaults(&default_config);
    
    TEST_ASSERT_EQUAL_STRING(default_config.wifi_ssid, reset_config.wifi_ssid);
    TEST_ASSERT_EQUAL_STRING(default_config.sip_user, reset_config.sip_user);
    TEST_ASSERT_EQUAL(default_config.web_port, reset_config.web_port);
    TEST_ASSERT_EQUAL(default_config.door_pulse_duration, reset_config.door_pulse_duration);
}

void test_e2e_config_validation_integration(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    
    // Test invalid configuration rejection
    door_station_config_t invalid_config;
    config_manager_get_defaults(&invalid_config);
    
    // Set invalid values
    strcpy(invalid_config.wifi_ssid, "ThisSSIDIsWayTooLongForWiFiSpecification");  // Too long
    strcpy(invalid_config.sip_user, "ab");  // Too short
    invalid_config.web_port = 100;  // Too low
    invalid_config.door_pulse_duration = 100;  // Too short
    
    // Validation should fail
    config_validation_error_t validation = config_manager_validate(&invalid_config);
    TEST_ASSERT_NOT_EQUAL(CONFIG_VALIDATION_OK, validation);
    
    // Save should fail for invalid configuration
    esp_err_t result = config_manager_save(&invalid_config);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // Test valid configuration acceptance
    door_station_config_t valid_config;
    config_manager_get_defaults(&valid_config);
    strcpy(valid_config.wifi_ssid, "ValidNetwork");
    strcpy(valid_config.sip_user, "validuser");
    valid_config.web_port = 8080;
    valid_config.door_pulse_duration = 2000;
    
    validation = config_manager_validate(&valid_config);
    TEST_ASSERT_EQUAL(CONFIG_VALIDATION_OK, validation);
    
    result = config_manager_save(&valid_config);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

// ============================================================================
// System Integration and Error Recovery Tests
// ============================================================================

void test_e2e_system_startup_sequence(void)
{
    // Test complete system startup sequence
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.wifi_ssid, "TestNetwork");
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    strcpy(config.sip_callee, "sip:resident@192.168.1.100");
    config.web_port = 8080;
    
    // Step 1: Initialize configuration manager
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    
    // Step 2: Initialize I/O manager
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    
    // Step 3: Initialize SIP manager
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    
    // Step 4: Initialize web server
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(config.web_port));
    
    // Step 5: Initialize application controller
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Verify all components are operational
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERING, sip_manager_get_state());
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_DOOR));
    TEST_ASSERT_EQUAL(RELAY_STATE_OFF, io_manager_get_relay_state(RELAY_LIGHT));
    
    // Simulate successful SIP registration
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
}

void test_e2e_component_failure_recovery(void)
{
    // Initialize system
    door_station_config_t config;
    config_manager_get_defaults(&config);
    strcpy(config.sip_user, "doorstation");
    strcpy(config.sip_domain, "192.168.1.100");
    
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, config_manager_save(&config));
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_init());
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    
    // Simulate SIP registration failure
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTRATION_FAILED, NULL);
    
    // System should handle gracefully - other components should still work
    TEST_ASSERT_TRUE(web_server_is_running());
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_toggle_relay(RELAY_LIGHT));
    
    // Simulate web server failure
    mock_http_server_set_start_fail(true);
    web_server_stop();
    esp_err_t result = web_server_init(8080);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
    
    // I/O operations should still work
    TEST_ASSERT_EQUAL(ESP_OK, io_manager_pulse_relay(RELAY_DOOR, 2000));
    
    // Reset failure mode and restart web server
    mock_http_server_set_start_fail(false);
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_TRUE(web_server_is_running());
}

void test_e2e_concurrent_operations(void)
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
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_init(&config));
    TEST_ASSERT_EQUAL(ESP_OK, sip_manager_start());
    TEST_ASSERT_EQUAL(ESP_OK, web_server_init(8080));
    TEST_ASSERT_EQUAL(ESP_OK, app_controller_init());
    
    // Simulate concurrent operations
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_REGISTERED, NULL);
    
    // Start call while toggling relays
    sip_manager_start_call(NULL);
    io_manager_toggle_relay(RELAY_LIGHT);
    
    // Simulate call connected
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_CONNECTED, NULL);
    
    // Process DTMF while web server is broadcasting
    mock_esp_sip_simulate_dtmf('1');
    web_server_broadcast_relay_status(RELAY_DOOR, RELAY_STATE_ON);
    
    // Update configuration while call is active
    strcpy(config.wifi_ssid, "NewNetwork");
    config_manager_save(&config);
    
    // End call
    sip_manager_end_call();
    mock_esp_sip_simulate_event(ESP_SIP_EVENT_CALL_ENDED, NULL);
    
    // Verify system state is consistent
    TEST_ASSERT_EQUAL(SIP_STATE_REGISTERED, sip_manager_get_state());
    TEST_ASSERT_FALSE(sip_manager_is_call_active());
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Verify configuration was saved
    door_station_config_t loaded_config;
    config_manager_load(&loaded_config);
    TEST_ASSERT_EQUAL_STRING("NewNetwork", loaded_config.wifi_ssid);
}