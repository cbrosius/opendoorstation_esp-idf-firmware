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
    
    UNITY_END();
}