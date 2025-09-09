#include "mock_http_server.h"
#include <string.h>

static mock_http_server_control_t s_http_control;

void mock_http_server_init(void)
{
    mock_http_server_reset();
}

void mock_http_server_reset(void)
{
    memset(&s_http_control, 0, sizeof(s_http_control));
    s_http_control.start_should_fail = false;
    s_http_control.stop_should_fail = false;
}

void mock_http_server_set_start_fail(bool fail)
{
    s_http_control.start_should_fail = fail;
}

void mock_http_server_set_stop_fail(bool fail)
{
    s_http_control.stop_should_fail = fail;
}

mock_http_server_control_t* mock_http_server_get_control(void)
{
    return &s_http_control;
}

// Mock implementations of ESP HTTP server functions
esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config)
{
    s_http_control.start_call_count++;
    
    if (s_http_control.start_should_fail) {
        return ESP_FAIL;
    }
    
    if (handle) {
        *handle = (httpd_handle_t)0x20000000;
        s_http_control.last_server_handle = *handle;
    }
    
    if (config) {
        s_http_control.last_port = config->server_port;
    }
    
    return ESP_OK;
}

esp_err_t httpd_stop(httpd_handle_t handle)
{
    s_http_control.stop_call_count++;
    
    if (s_http_control.stop_should_fail) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler)
{
    s_http_control.register_uri_call_count++;
    
    if (uri_handler) {
        strncpy(s_http_control.last_uri, uri_handler->uri, sizeof(s_http_control.last_uri) - 1);
        s_http_control.last_method = uri_handler->method;
    }
    
    return ESP_OK;
}

esp_err_t httpd_unregister_uri_handler(httpd_handle_t handle, const char *uri, httpd_method_t method)
{
    return ESP_OK;
}

esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, ssize_t buf_len)
{
    return ESP_OK;
}

esp_err_t httpd_resp_send_chunk(httpd_req_t *r, const char *buf, ssize_t buf_len)
{
    return ESP_OK;
}

esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type)
{
    return ESP_OK;
}

esp_err_t httpd_resp_set_hdr(httpd_req_t *r, const char *field, const char *value)
{
    return ESP_OK;
}