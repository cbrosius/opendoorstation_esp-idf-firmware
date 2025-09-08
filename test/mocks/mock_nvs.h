#ifndef MOCK_NVS_H
#define MOCK_NVS_H

#include "esp_err.h"
#include "nvs.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock NVS storage structure
 */
typedef struct {
    char key[16];
    char str_value[128];
    uint16_t u16_value;
    uint32_t u32_value;
    bool is_string;
    bool is_u16;
    bool is_u32;
    bool exists;
} mock_nvs_entry_t;

/**
 * @brief Initialize mock NVS system
 */
void mock_nvs_init(void);

/**
 * @brief Clear all mock NVS entries
 */
void mock_nvs_clear(void);

/**
 * @brief Set mock NVS to simulate failure
 */
void mock_nvs_set_fail_mode(bool fail);

/**
 * @brief Get number of entries in mock NVS
 */
int mock_nvs_get_entry_count(void);

/**
 * @brief Check if a key exists in mock NVS
 */
bool mock_nvs_key_exists(const char* key);

#ifdef __cplusplus
}
#endif

#endif // MOCK_NVS_H