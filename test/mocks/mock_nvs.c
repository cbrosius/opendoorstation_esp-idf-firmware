#include "mock_nvs.h"
#include <string.h>
#include <stdio.h>

#define MAX_MOCK_ENTRIES 32

static mock_nvs_entry_t mock_entries[MAX_MOCK_ENTRIES];
static int entry_count = 0;
static bool fail_mode = false;
static bool nvs_initialized = false;

void mock_nvs_init(void) {
    mock_nvs_clear();
    nvs_initialized = true;
    fail_mode = false;
}

void mock_nvs_clear(void) {
    memset(mock_entries, 0, sizeof(mock_entries));
    entry_count = 0;
}

void mock_nvs_set_fail_mode(bool fail) {
    fail_mode = fail;
}

int mock_nvs_get_entry_count(void) {
    return entry_count;
}

bool mock_nvs_key_exists(const char* key) {
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0 && mock_entries[i].exists) {
            return true;
        }
    }
    return false;
}

static mock_nvs_entry_t* find_or_create_entry(const char* key) {
    // Find existing entry
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0) {
            return &mock_entries[i];
        }
    }
    
    // Create new entry if space available
    if (entry_count < MAX_MOCK_ENTRIES) {
        mock_nvs_entry_t* entry = &mock_entries[entry_count++];
        strncpy(entry->key, key, sizeof(entry->key) - 1);
        entry->key[sizeof(entry->key) - 1] = '\0';
        return entry;
    }
    
    return NULL;
}

// Mock NVS Flash functions
esp_err_t nvs_flash_init(void) {
    if (fail_mode) {
        return ESP_ERR_NVS_NO_FREE_PAGES;
    }
    nvs_initialized = true;
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void) {
    if (fail_mode) {
        return ESP_FAIL;
    }
    mock_nvs_clear();
    return ESP_OK;
}

// Mock NVS Handle functions
esp_err_t nvs_open(const char* namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    if (fail_mode || !nvs_initialized) {
        return ESP_ERR_NVS_NOT_INITIALIZED;
    }
    
    // Return a dummy handle (just use 1 for simplicity)
    *out_handle = 1;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
    // Nothing to do in mock
    (void)handle;
}

// Mock NVS String functions
esp_err_t nvs_set_str(nvs_handle_t handle, const char* key, const char* value) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    }
    
    mock_nvs_entry_t* entry = find_or_create_entry(key);
    if (entry == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    strncpy(entry->str_value, value, sizeof(entry->str_value) - 1);
    entry->str_value[sizeof(entry->str_value) - 1] = '\0';
    entry->is_string = true;
    entry->exists = true;
    
    return ESP_OK;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char* key, char* out_value, size_t* length) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0 && 
            mock_entries[i].exists && 
            mock_entries[i].is_string) {
            
            size_t str_len = strlen(mock_entries[i].str_value) + 1;
            if (*length < str_len) {
                *length = str_len;
                return ESP_ERR_NVS_INVALID_LENGTH;
            }
            
            strcpy(out_value, mock_entries[i].str_value);
            *length = str_len;
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}

// Mock NVS U16 functions
esp_err_t nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    }
    
    mock_nvs_entry_t* entry = find_or_create_entry(key);
    if (entry == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    entry->u16_value = value;
    entry->is_u16 = true;
    entry->exists = true;
    
    return ESP_OK;
}

esp_err_t nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0 && 
            mock_entries[i].exists && 
            mock_entries[i].is_u16) {
            
            *out_value = mock_entries[i].u16_value;
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}

// Mock NVS U32 functions
esp_err_t nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_ENOUGH_SPACE;
    }
    
    mock_nvs_entry_t* entry = find_or_create_entry(key);
    if (entry == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    entry->u32_value = value;
    entry->is_u32 = true;
    entry->exists = true;
    
    return ESP_OK;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value) {
    if (fail_mode) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0 && 
            mock_entries[i].exists && 
            mock_entries[i].is_u32) {
            
            *out_value = mock_entries[i].u32_value;
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}

// Mock NVS Commit and Erase functions
esp_err_t nvs_commit(nvs_handle_t handle) {
    if (fail_mode) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t nvs_erase_all(nvs_handle_t handle) {
    if (fail_mode) {
        return ESP_FAIL;
    }
    
    // Mark all entries as non-existent
    for (int i = 0; i < entry_count; i++) {
        mock_entries[i].exists = false;
    }
    
    return ESP_OK;
}esp_e
rr_t nvs_erase_key(nvs_handle_t handle, const char* key) {
    if (fail_mode) {
        return ESP_FAIL;
    }
    
    for (int i = 0; i < entry_count; i++) {
        if (strcmp(mock_entries[i].key, key) == 0) {
            mock_entries[i].exists = false;
            return ESP_OK;
        }
    }
    
    return ESP_ERR_NVS_NOT_FOUND;
}