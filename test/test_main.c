#include "unity.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "test_main";

// External test function declarations
extern void test_config_validation_valid_config(void);
extern void test_config_validation_wifi_ssid_too_long(void);
extern void test_config_validation_wifi_ssid_empty_valid(void);
extern void test_config_validation_sip_user_too_short(void);
extern void test_config_validation_sip_user_too_long(void);
extern void test_config_validation_sip_user_invalid_chars(void);
extern void test_config_validation_sip_user_valid_underscore(void);
extern void test_config_validation_sip_domain_ipv4_valid(void);
extern void test_config_validation_sip_domain_hostname_valid(void);
extern void test_config_validation_sip_domain_invalid(void);
extern void test_config_validation_sip_callee_valid_simple(void);
extern void test_config_validation_sip_callee_valid_with_sip_prefix(void);
extern void test_config_validation_sip_callee_invalid_no_at(void);
extern void test_config_validation_web_port_too_low(void);
extern void test_config_validation_web_port_too_high(void);
extern void test_config_validation_web_port_valid_range(void);
extern void test_config_validation_door_pulse_too_short(void);
extern void test_config_validation_door_pulse_too_long(void);
extern void test_config_validation_door_pulse_valid_range(void);
extern void test_config_get_defaults(void);
extern void test_config_validation_error_messages(void);
extern void test_config_validation_null_pointer(void);

// Storage test function declarations
extern void test_config_save_and_load_success(void);
extern void test_config_load_defaults_when_nvs_empty(void);
extern void test_config_save_invalid_config_fails(void);
extern void test_config_save_nvs_failure(void);
extern void test_config_load_nvs_failure(void);
extern void test_config_factory_reset(void);
extern void test_config_factory_reset_nvs_failure(void);
extern void test_config_save_load_null_pointer(void);
extern void test_config_partial_load_with_missing_keys(void);

// Environment variable test function declarations
extern void test_config_init_with_empty_nvs(void);
extern void test_config_init_with_existing_nvs_data(void);
extern void test_config_init_nvs_failure_recovery(void);
extern void test_config_init_saves_merged_config(void);
extern void test_config_init_multiple_times(void);
extern void test_config_init_with_nvs_corruption(void);
extern void test_config_validation_after_init(void);
extern void test_config_init_preserves_valid_runtime_config(void);

// SIP manager test function declarations
extern void test_sip_manager_init_success(void);
extern void test_sip_manager_init_invalid_config(void);
extern void test_sip_manager_init_esp_sip_failure(void);
extern void test_sip_manager_start_success(void);
extern void test_sip_manager_start_not_initialized(void);
extern void test_sip_manager_start_esp_sip_failure(void);
extern void test_sip_manager_start_call_success(void);
extern void test_sip_manager_start_call_default_callee(void);
extern void test_sip_manager_start_call_not_registered(void);
extern void test_sip_manager_end_call_success(void);
extern void test_sip_manager_dtmf_callback_registration(void);
extern void test_sip_manager_call_duration(void);
extern void test_sip_manager_update_config(void);
extern void test_sip_manager_registration_failure(void);
extern void test_sip_manager_call_failure(void);
extern void test_sip_manager_call_statistics_tracking(void);
extern void test_sip_manager_call_failure_statistics(void);
extern void test_sip_manager_reset_call_statistics(void);
extern void test_sip_manager_call_timeout_handling(void);
extern void test_sip_manager_get_call_stats_invalid_args(void);
extern void test_sip_manager_reset_call_stats_not_initialized(void);
extern void test_sip_manager_dtmf_command_mapping_default(void);
extern void test_sip_manager_dtmf_command_mapping_custom(void);
extern void test_sip_manager_dtmf_processing_disabled(void);
extern void test_sip_manager_get_dtmf_commands(void);
extern void test_sip_manager_configure_dtmf_commands_invalid_args(void);
extern void test_sip_manager_dtmf_command_callback_not_initialized(void);

// SIP-IO integration test function declarations
extern void test_sip_io_integration_init_success(void);
extern void test_sip_io_integration_init_invalid_config(void);
extern void test_sip_io_integration_start_success(void);
extern void test_sip_io_integration_start_not_initialized(void);
extern void test_sip_io_integration_dtmf_door_open(void);
extern void test_sip_io_integration_dtmf_custom_pulse_duration(void);
extern void test_sip_io_integration_auto_hangup_after_door_open(void);
extern void test_sip_io_integration_auto_hangup_disabled(void);
extern void test_sip_io_integration_dtmf_hangup_command(void);
extern void test_sip_io_integration_button_press(void);
extern void test_sip_io_integration_status_request(void);
extern void test_sip_io_integration_update_config(void);
extern void test_sip_io_integration_inactive_ignores_commands(void);

void setUp(void) {
    // Set up code for each test
}

void tearDown(void) {
    // Clean up code for each test
}

void test_basic_functionality(void) {
    ESP_LOGI(TAG, "Running basic functionality test");
    TEST_ASSERT_TRUE(true);
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Unity test framework");
    
    UNITY_BEGIN();
    
    // Basic functionality test
    RUN_TEST(test_basic_functionality);
    
    // Configuration validation tests
    RUN_TEST(test_config_validation_valid_config);
    RUN_TEST(test_config_validation_wifi_ssid_too_long);
    RUN_TEST(test_config_validation_wifi_ssid_empty_valid);
    RUN_TEST(test_config_validation_sip_user_too_short);
    RUN_TEST(test_config_validation_sip_user_too_long);
    RUN_TEST(test_config_validation_sip_user_invalid_chars);
    RUN_TEST(test_config_validation_sip_user_valid_underscore);
    RUN_TEST(test_config_validation_sip_domain_ipv4_valid);
    RUN_TEST(test_config_validation_sip_domain_hostname_valid);
    RUN_TEST(test_config_validation_sip_domain_invalid);
    RUN_TEST(test_config_validation_sip_callee_valid_simple);
    RUN_TEST(test_config_validation_sip_callee_valid_with_sip_prefix);
    RUN_TEST(test_config_validation_sip_callee_invalid_no_at);
    RUN_TEST(test_config_validation_web_port_too_low);
    RUN_TEST(test_config_validation_web_port_too_high);
    RUN_TEST(test_config_validation_web_port_valid_range);
    RUN_TEST(test_config_validation_door_pulse_too_short);
    RUN_TEST(test_config_validation_door_pulse_too_long);
    RUN_TEST(test_config_validation_door_pulse_valid_range);
    RUN_TEST(test_config_get_defaults);
    RUN_TEST(test_config_validation_error_messages);
    RUN_TEST(test_config_validation_null_pointer);
    
    // Configuration storage tests
    RUN_TEST(test_config_save_and_load_success);
    RUN_TEST(test_config_load_defaults_when_nvs_empty);
    RUN_TEST(test_config_save_invalid_config_fails);
    RUN_TEST(test_config_save_nvs_failure);
    RUN_TEST(test_config_load_nvs_failure);
    RUN_TEST(test_config_factory_reset);
    RUN_TEST(test_config_factory_reset_nvs_failure);
    RUN_TEST(test_config_save_load_null_pointer);
    RUN_TEST(test_config_partial_load_with_missing_keys);
    
    // Environment variable integration tests
    RUN_TEST(test_config_init_with_empty_nvs);
    RUN_TEST(test_config_init_with_existing_nvs_data);
    RUN_TEST(test_config_init_nvs_failure_recovery);
    RUN_TEST(test_config_init_saves_merged_config);
    RUN_TEST(test_config_init_multiple_times);
    RUN_TEST(test_config_init_with_nvs_corruption);
    RUN_TEST(test_config_validation_after_init);
    RUN_TEST(test_config_init_preserves_valid_runtime_config);
    
    // SIP manager tests
    RUN_TEST(test_sip_manager_init_success);
    RUN_TEST(test_sip_manager_init_invalid_config);
    RUN_TEST(test_sip_manager_init_esp_sip_failure);
    RUN_TEST(test_sip_manager_start_success);
    RUN_TEST(test_sip_manager_start_not_initialized);
    RUN_TEST(test_sip_manager_start_esp_sip_failure);
    RUN_TEST(test_sip_manager_start_call_success);
    RUN_TEST(test_sip_manager_start_call_default_callee);
    RUN_TEST(test_sip_manager_start_call_not_registered);
    RUN_TEST(test_sip_manager_end_call_success);
    RUN_TEST(test_sip_manager_dtmf_callback_registration);
    RUN_TEST(test_sip_manager_call_duration);
    RUN_TEST(test_sip_manager_update_config);
    RUN_TEST(test_sip_manager_registration_failure);
    RUN_TEST(test_sip_manager_call_failure);
    RUN_TEST(test_sip_manager_call_statistics_tracking);
    RUN_TEST(test_sip_manager_call_failure_statistics);
    RUN_TEST(test_sip_manager_reset_call_statistics);
    RUN_TEST(test_sip_manager_call_timeout_handling);
    RUN_TEST(test_sip_manager_get_call_stats_invalid_args);
    RUN_TEST(test_sip_manager_reset_call_stats_not_initialized);
    RUN_TEST(test_sip_manager_dtmf_command_mapping_default);
    RUN_TEST(test_sip_manager_dtmf_command_mapping_custom);
    RUN_TEST(test_sip_manager_dtmf_processing_disabled);
    RUN_TEST(test_sip_manager_get_dtmf_commands);
    RUN_TEST(test_sip_manager_configure_dtmf_commands_invalid_args);
    RUN_TEST(test_sip_manager_dtmf_command_callback_not_initialized);
    
    // SIP-IO integration tests
    RUN_TEST(test_sip_io_integration_init_success);
    RUN_TEST(test_sip_io_integration_init_invalid_config);
    RUN_TEST(test_sip_io_integration_start_success);
    RUN_TEST(test_sip_io_integration_start_not_initialized);
    RUN_TEST(test_sip_io_integration_dtmf_door_open);
    RUN_TEST(test_sip_io_integration_dtmf_custom_pulse_duration);
    RUN_TEST(test_sip_io_integration_auto_hangup_after_door_open);
    RUN_TEST(test_sip_io_integration_auto_hangup_disabled);
    RUN_TEST(test_sip_io_integration_dtmf_hangup_command);
    RUN_TEST(test_sip_io_integration_button_press);
    RUN_TEST(test_sip_io_integration_status_request);
    RUN_TEST(test_sip_io_integration_update_config);
    RUN_TEST(test_sip_io_integration_inactive_ignores_commands);
    
    UNITY_END();
}