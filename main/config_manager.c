#include "config_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <arpa/inet.h>

static const char *TAG = "config_manager";
static const char *NVS_NAMESPACE = "door_station";
static const char *NVS_SECURE_NAMESPACE = "door_secure";

// Sensitive field names that should be encrypted and masked
static const char* SENSITIVE_FIELDS[] = {
    "wifi_password",
    "sip_password"
};
static const size_t SENSITIVE_FIELDS_COUNT = sizeof(SENSITIVE_FIELDS) / sizeof(SENSITIVE_FIELDS[0]);

// Mask character for sensitive values
static const char MASK_CHAR = '*';
static const size_t MIN_MASK_LENGTH = 8;

// Current merged configuration (NVS + build-time overrides)
static door_station_config_t current_merged_config = {0};
static bool config_manager_initialized = false;

// Default configuration values
static const door_station_config_t DEFAULT_CONFIG = {
    .wifi_ssid = "",
    .wifi_password = "",
    .sip_user = "",
    .sip_domain = "",
    .sip_password = "",
    .sip_callee = "",
    .web_port = 8080,
    .door_pulse_duration = 2000
};

// Validation error messages
static const char* validation_error_messages[] = {
    [CONFIG_VALIDATION_OK] = "Configuration is valid",
    [CONFIG_VALIDATION_WIFI_SSID_INVALID] = "Wi-Fi SSID contains invalid characters",
    [CONFIG_VALIDATION_WIFI_SSID_TOO_LONG] = "Wi-Fi SSID is too long (max 31 characters)",
    [CONFIG_VALIDATION_SIP_USER_INVALID] = "SIP user contains invalid characters (alphanumeric and underscore only)",
    [CONFIG_VALIDATION_SIP_USER_TOO_SHORT] = "SIP user is too short (minimum 3 characters)",
    [CONFIG_VALIDATION_SIP_USER_TOO_LONG] = "SIP user is too long (maximum 31 characters)",
    [CONFIG_VALIDATION_SIP_DOMAIN_INVALID] = "SIP domain is not a valid hostname or IP address",
    [CONFIG_VALIDATION_SIP_CALLEE_INVALID] = "SIP callee is not a valid SIP URI",
    [CONFIG_VALIDATION_WEB_PORT_INVALID] = "Web port must be between 1024 and 65535",
    [CONFIG_VALIDATION_DOOR_PULSE_INVALID] = "Door pulse duration must be between 500 and 10000 ms"
};

/**
 * @brief Check if string contains only valid Wi-Fi SSID characters
 */
static bool is_valid_wifi_ssid(const char *ssid) {
    if (strlen(ssid) == 0 || strlen(ssid) > 31) {
        return false;
    }
    
    // Wi-Fi SSID can contain most printable characters except some special ones
    for (int i = 0; ssid[i] != '\0'; i++) {
        char c = ssid[i];
        if (c < 32 || c > 126) {  // Non-printable characters
            return false;
        }
    }
    return true;
}

/**
 * @brief Check if string contains only alphanumeric characters and underscores
 */
static bool is_valid_sip_user(const char *user) {
    size_t len = strlen(user);
    if (len < 3 || len > 31) {
        return false;
    }
    
    for (int i = 0; user[i] != '\0'; i++) {
        char c = user[i];
        if (!isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}

/**
 * @brief Check if string is a valid hostname or IP address
 */
static bool is_valid_domain(const char *domain) {
    if (strlen(domain) == 0 || strlen(domain) > 63) {
        return false;
    }
    
    // Try to parse as IPv4 address
    struct in_addr addr;
    if (inet_aton(domain, &addr) == 1) {
        return true;  // Valid IPv4 address
    }
    
    // Check as hostname (simplified validation)
    for (int i = 0; domain[i] != '\0'; i++) {
        char c = domain[i];
        if (!isalnum(c) && c != '.' && c != '-') {
            return false;
        }
    }
    
    // Must not start or end with hyphen or dot
    if (domain[0] == '-' || domain[0] == '.' || 
        domain[strlen(domain)-1] == '-' || domain[strlen(domain)-1] == '.') {
        return false;
    }
    
    return true;
}

/**
 * @brief Check if string is a valid SIP URI format
 */
static bool is_valid_sip_uri(const char *uri) {
    if (strlen(uri) == 0 || strlen(uri) > 63) {
        return false;
    }
    
    // Simple SIP URI validation: user@domain or sip:user@domain
    const char *at_pos = strchr(uri, '@');
    if (at_pos == NULL) {
        return false;  // Must contain @
    }
    
    // Check if it starts with "sip:"
    const char *user_start = uri;
    if (strncmp(uri, "sip:", 4) == 0) {
        user_start = uri + 4;
    }
    
    // Validate user part (before @)
    size_t user_len = at_pos - user_start;
    if (user_len == 0 || user_len > 31) {
        return false;
    }
    
    char user_part[32];
    strncpy(user_part, user_start, user_len);
    user_part[user_len] = '\0';
    
    if (!is_valid_sip_user(user_part)) {
        return false;
    }
    
    // Validate domain part (after @)
    const char *domain_part = at_pos + 1;
    return is_valid_domain(domain_part);
}

/**
 * @brief Initialize secure NVS partition for sensitive data
 */
static esp_err_t init_secure_nvs(void) {
    ESP_LOGI(TAG, "Initializing secure NVS partition");
    
    // For now, we'll use the default NVS encryption
    // In production, you would use a dedicated secure partition
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    return ret;
}

/**
 * @brief Save sensitive string to secure storage
 */
static esp_err_t save_sensitive_string(nvs_handle_t handle, const char *key, const char *value) {
    if (strlen(value) == 0) {
        // Delete key if value is empty
        esp_err_t err = nvs_erase_key(handle, key);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            return ESP_OK; // Key doesn't exist, that's fine
        }
        return err;
    }
    
    return nvs_set_str(handle, key, value);
}

/**
 * @brief Load sensitive string from secure storage
 */
static esp_err_t load_sensitive_string(nvs_handle_t handle, const char *key, char *value, size_t max_size) {
    size_t required_size = max_size;
    esp_err_t err = nvs_get_str(handle, key, value, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        value[0] = '\0'; // Set to empty string if not found
        return ESP_OK;
    }
    return err;
}

/**
 * @brief Apply build-time configuration from compile definitions
 */
static void apply_build_time_config(door_station_config_t *config) {
    ESP_LOGI(TAG, "Applying build-time configuration");
    
    // Apply Wi-Fi configuration from build-time
    #ifdef CONFIG_WIFI_SSID
    strncpy(config->wifi_ssid, CONFIG_WIFI_SSID, sizeof(config->wifi_ssid) - 1);
    config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time Wi-Fi SSID: %s", config->wifi_ssid);
    #endif
    
    #ifdef CONFIG_WIFI_PASSWORD
    strncpy(config->wifi_password, CONFIG_WIFI_PASSWORD, sizeof(config->wifi_password) - 1);
    config->wifi_password[sizeof(config->wifi_password) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time Wi-Fi password configured");
    #endif
    
    // Apply SIP configuration from build-time
    #ifdef CONFIG_SIP_USER
    strncpy(config->sip_user, CONFIG_SIP_USER, sizeof(config->sip_user) - 1);
    config->sip_user[sizeof(config->sip_user) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time SIP user: %s", config->sip_user);
    #endif
    
    #ifdef CONFIG_SIP_DOMAIN
    strncpy(config->sip_domain, CONFIG_SIP_DOMAIN, sizeof(config->sip_domain) - 1);
    config->sip_domain[sizeof(config->sip_domain) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time SIP domain: %s", config->sip_domain);
    #endif
    
    #ifdef CONFIG_SIP_PASSWORD
    strncpy(config->sip_password, CONFIG_SIP_PASSWORD, sizeof(config->sip_password) - 1);
    config->sip_password[sizeof(config->sip_password) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time SIP password configured");
    #endif
    
    #ifdef CONFIG_SIP_CALLEE
    strncpy(config->sip_callee, CONFIG_SIP_CALLEE, sizeof(config->sip_callee) - 1);
    config->sip_callee[sizeof(config->sip_callee) - 1] = '\0';
    ESP_LOGI(TAG, "Build-time SIP callee: %s", config->sip_callee);
    #endif
    
    // Apply numeric configuration from build-time
    #ifdef CONFIG_WEB_PORT
    config->web_port = CONFIG_WEB_PORT;
    ESP_LOGI(TAG, "Build-time web port: %" PRIu16, config->web_port);
    #endif
    
    #ifdef CONFIG_DOOR_PULSE_DURATION
    config->door_pulse_duration = CONFIG_DOOR_PULSE_DURATION;
    ESP_LOGI(TAG, "Build-time door pulse duration: %" PRIu32 " ms", config->door_pulse_duration);
    #endif
}

esp_err_t config_manager_init(void) {
    ESP_LOGI(TAG, "Initializing configuration manager");
    
    // Initialize secure NVS
    esp_err_t ret = init_secure_nvs();
    ESP_ERROR_CHECK(ret);
    
    // Load current configuration from NVS
    door_station_config_t current_config;
    ret = config_manager_load(&current_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load configuration from NVS, using defaults");
        config_manager_get_defaults(&current_config);
    }
    
    // Apply build-time configuration (overrides NVS values)
    apply_build_time_config(&current_config);
    
    // Validate the merged configuration
    config_validation_error_t validation_result = config_manager_validate(&current_config);
    if (validation_result != CONFIG_VALIDATION_OK) {
        ESP_LOGW(TAG, "Merged configuration validation failed: %s", 
                 config_manager_get_validation_error_message(validation_result));
        ESP_LOGW(TAG, "Using defaults for invalid fields");
        
        // Reset to defaults and reapply only valid build-time config
        door_station_config_t defaults;
        config_manager_get_defaults(&defaults);
        apply_build_time_config(&defaults);
        
        // Validate again
        validation_result = config_manager_validate(&defaults);
        if (validation_result == CONFIG_VALIDATION_OK) {
            current_config = defaults;
        } else {
            ESP_LOGE(TAG, "Build-time configuration is invalid, using factory defaults");
            config_manager_get_defaults(&current_config);
        }
    }
    
    // Save the merged configuration back to NVS (if it's different)
    door_station_config_t nvs_config;
    esp_err_t load_result = config_manager_load(&nvs_config);
    if (load_result != ESP_OK || memcmp(&current_config, &nvs_config, sizeof(door_station_config_t)) != 0) {
        ESP_LOGI(TAG, "Saving merged configuration to NVS");
        ret = config_manager_save(&current_config);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save merged configuration to NVS");
        }
    }
    
    // Store the merged configuration for later retrieval
    current_merged_config = current_config;
    config_manager_initialized = true;
    
    ESP_LOGI(TAG, "Configuration manager initialized successfully");
    return ESP_OK;
}

config_validation_error_t config_manager_validate(const door_station_config_t *config) {
    if (config == NULL) {
        return CONFIG_VALIDATION_WIFI_SSID_INVALID;
    }
    
    // Validate Wi-Fi SSID
    if (strlen(config->wifi_ssid) > 31) {
        return CONFIG_VALIDATION_WIFI_SSID_TOO_LONG;
    }
    if (strlen(config->wifi_ssid) > 0 && !is_valid_wifi_ssid(config->wifi_ssid)) {
        return CONFIG_VALIDATION_WIFI_SSID_INVALID;
    }
    
    // Validate SIP user
    if (strlen(config->sip_user) > 0) {
        if (strlen(config->sip_user) < 3) {
            return CONFIG_VALIDATION_SIP_USER_TOO_SHORT;
        }
        if (strlen(config->sip_user) > 31) {
            return CONFIG_VALIDATION_SIP_USER_TOO_LONG;
        }
        if (!is_valid_sip_user(config->sip_user)) {
            return CONFIG_VALIDATION_SIP_USER_INVALID;
        }
    }
    
    // Validate SIP domain
    if (strlen(config->sip_domain) > 0 && !is_valid_domain(config->sip_domain)) {
        return CONFIG_VALIDATION_SIP_DOMAIN_INVALID;
    }
    
    // Validate SIP callee
    if (strlen(config->sip_callee) > 0 && !is_valid_sip_uri(config->sip_callee)) {
        return CONFIG_VALIDATION_SIP_CALLEE_INVALID;
    }
    
    // Validate web port
    if (config->web_port < 1024) {
        return CONFIG_VALIDATION_WEB_PORT_INVALID;
    }
    
    // Validate door pulse duration
    if (config->door_pulse_duration < 500 || config->door_pulse_duration > 10000) {
        return CONFIG_VALIDATION_DOOR_PULSE_INVALID;
    }
    
    return CONFIG_VALIDATION_OK;
}

void config_manager_get_defaults(door_station_config_t *config) {
    if (config != NULL) {
        memcpy(config, &DEFAULT_CONFIG, sizeof(door_station_config_t));
    }
}

const char* config_manager_get_validation_error_message(config_validation_error_t error) {
    if (error >= 0 && error < sizeof(validation_error_messages) / sizeof(validation_error_messages[0])) {
        return validation_error_messages[error];
    }
    return "Unknown validation error";
}

esp_err_t config_manager_load(door_station_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Loading configuration from NVS");
    
    // Start with defaults
    config_manager_get_defaults(config);
    
    // Load non-sensitive configuration from regular NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err == ESP_OK) {
        size_t required_size;
        
        required_size = sizeof(config->wifi_ssid);
        nvs_get_str(nvs_handle, "wifi_ssid", config->wifi_ssid, &required_size);
        
        required_size = sizeof(config->sip_user);
        nvs_get_str(nvs_handle, "sip_user", config->sip_user, &required_size);
        
        required_size = sizeof(config->sip_domain);
        nvs_get_str(nvs_handle, "sip_domain", config->sip_domain, &required_size);
        
        required_size = sizeof(config->sip_callee);
        nvs_get_str(nvs_handle, "sip_callee", config->sip_callee, &required_size);
        
        nvs_get_u16(nvs_handle, "web_port", &config->web_port);
        nvs_get_u32(nvs_handle, "door_pulse_duration", &config->door_pulse_duration);
        
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Failed to open regular NVS namespace");
    }
    
    // Load sensitive configuration from secure NVS
    nvs_handle_t secure_handle;
    err = nvs_open(NVS_SECURE_NAMESPACE, NVS_READONLY, &secure_handle);
    if (err == ESP_OK) {
        load_sensitive_string(secure_handle, "wifi_password", config->wifi_password, sizeof(config->wifi_password));
        load_sensitive_string(secure_handle, "sip_password", config->sip_password, sizeof(config->sip_password));
        
        nvs_close(secure_handle);
        ESP_LOGI(TAG, "Sensitive configuration loaded from secure storage");
    } else {
        ESP_LOGW(TAG, "Failed to open secure NVS namespace, sensitive fields will be empty");
    }
    
    ESP_LOGI(TAG, "Configuration loaded successfully");
    return ESP_OK;
}

esp_err_t config_manager_save(const door_station_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate configuration before saving
    config_validation_error_t validation_result = config_manager_validate(config);
    if (validation_result != CONFIG_VALIDATION_OK) {
        ESP_LOGE(TAG, "Configuration validation failed: %s", 
                 config_manager_get_validation_error_message(validation_result));
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Saving configuration to NVS");
    
    esp_err_t err = ESP_OK;
    
    // Save non-sensitive configuration to regular NVS
    nvs_handle_t nvs_handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace for writing");
        return err;
    }
    
    err |= nvs_set_str(nvs_handle, "wifi_ssid", config->wifi_ssid);
    err |= nvs_set_str(nvs_handle, "sip_user", config->sip_user);
    err |= nvs_set_str(nvs_handle, "sip_domain", config->sip_domain);
    err |= nvs_set_str(nvs_handle, "sip_callee", config->sip_callee);
    err |= nvs_set_u16(nvs_handle, "web_port", config->web_port);
    err |= nvs_set_u32(nvs_handle, "door_pulse_duration", config->door_pulse_duration);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save non-sensitive configuration to NVS");
        return err;
    }
    
    // Save sensitive configuration to secure NVS
    nvs_handle_t secure_handle;
    err = nvs_open(NVS_SECURE_NAMESPACE, NVS_READWRITE, &secure_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open secure NVS namespace for writing");
        return err;
    }
    
    err |= save_sensitive_string(secure_handle, "wifi_password", config->wifi_password);
    err |= save_sensitive_string(secure_handle, "sip_password", config->sip_password);
    
    if (err == ESP_OK) {
        err = nvs_commit(secure_handle);
    }
    
    nvs_close(secure_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save sensitive configuration to secure NVS");
        return err;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully to both regular and secure storage");
    return ESP_OK;
}

esp_err_t config_manager_factory_reset(void) {
    ESP_LOGI(TAG, "Performing factory reset");
    
    esp_err_t err = ESP_OK;
    
    // Clear regular NVS namespace
    nvs_handle_t nvs_handle;
    esp_err_t regular_err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (regular_err == ESP_OK) {
        regular_err = nvs_erase_all(nvs_handle);
        if (regular_err == ESP_OK) {
            regular_err = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
        
        if (regular_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear regular NVS namespace");
            err = regular_err;
        } else {
            ESP_LOGI(TAG, "Regular configuration cleared");
        }
    } else {
        ESP_LOGW(TAG, "Failed to open regular NVS namespace for factory reset");
    }
    
    // Clear secure NVS namespace
    nvs_handle_t secure_handle;
    esp_err_t secure_err = nvs_open(NVS_SECURE_NAMESPACE, NVS_READWRITE, &secure_handle);
    if (secure_err == ESP_OK) {
        secure_err = nvs_erase_all(secure_handle);
        if (secure_err == ESP_OK) {
            secure_err = nvs_commit(secure_handle);
        }
        nvs_close(secure_handle);
        
        if (secure_err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear secure NVS namespace");
            err = secure_err;
        } else {
            ESP_LOGI(TAG, "Secure credentials cleared");
        }
    } else {
        ESP_LOGW(TAG, "Failed to open secure NVS namespace for factory reset");
    }
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Factory reset completed successfully");
    } else {
        ESP_LOGE(TAG, "Factory reset completed with errors");
    }
    
    return err;
}

esp_err_t config_manager_load_masked(door_station_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // First load the actual configuration
    esp_err_t err = config_manager_load(config);
    if (err != ESP_OK) {
        return err;
    }
    
    // Mask sensitive fields
    config_manager_mask_sensitive_value(config->wifi_password, config->wifi_password, sizeof(config->wifi_password));
    config_manager_mask_sensitive_value(config->sip_password, config->sip_password, sizeof(config->sip_password));
    
    ESP_LOGI(TAG, "Configuration loaded with masked sensitive fields");
    return ESP_OK;
}

bool config_manager_is_sensitive_field(const char *field_name) {
    if (field_name == NULL) {
        return false;
    }
    
    for (size_t i = 0; i < SENSITIVE_FIELDS_COUNT; i++) {
        if (strcmp(field_name, SENSITIVE_FIELDS[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

void config_manager_mask_sensitive_value(const char *value, char *masked_value, size_t masked_size) {
    if (value == NULL || masked_value == NULL || masked_size == 0) {
        return;
    }
    
    size_t value_len = strlen(value);
    
    if (value_len == 0) {
        // Empty value, keep it empty
        masked_value[0] = '\0';
        return;
    }
    
    // Calculate mask length (minimum 8 characters, or actual length if shorter)
    size_t mask_len = (value_len < MIN_MASK_LENGTH) ? value_len : MIN_MASK_LENGTH;
    
    // Ensure we don't exceed buffer size
    if (mask_len >= masked_size) {
        mask_len = masked_size - 1;
    }
    
    // Fill with mask characters
    for (size_t i = 0; i < mask_len; i++) {
        masked_value[i] = MASK_CHAR;
    }
    
    // Null terminate
    masked_value[mask_len] = '\0';
}

esp_err_t config_manager_get_current(door_station_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!config_manager_initialized) {
        ESP_LOGE(TAG, "Configuration manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Return the current merged configuration
    *config = current_merged_config;
    return ESP_OK;
}