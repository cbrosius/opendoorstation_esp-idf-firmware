#ifndef MOCK_HTTP_SERVER_H
#define MOCK_HTTP_SERVER_H

#include "esp_http_server.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mock HTTP server control structure
 */
typedef struct {
    bool start_should_fail;
    bool stop_should_fail;
    int start_call_count;
    int stop_call_count;
    int register_uri_call_count;
    httpd_handle_t last_server_handle;
    uint16_t last_port;
    char last_uri[128];
    httpd_method_t last_method;
} mock_http_server_control_t;

/**
 * @brief Initialize mock HTTP server system
 */
void mock_http_server_init(void);

/**
 * @brief Reset mock HTTP server state
 */
void mock_http_server_reset(void);

/**
 * @brief Set mock HTTP server to simulate failures
 */
void mock_http_server_set_start_fail(bool fail);
void mock_http_server_set_stop_fail(bool fail);

/**
 * @brief Get mock HTTP server control structure
 */
mock_http_server_control_t* mock_http_server_get_control(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_HTTP_SERVER_H