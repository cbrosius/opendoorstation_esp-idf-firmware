#include "web_server.h"
#include "config_manager.h"
#include "io_manager.h"
#include "io_events.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "web_server";
static httpd_handle_t server = NULL;
static uint16_t server_port = 0;

// Helper function to get and log IP address
static void log_web_interface_url(uint16_t port) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "=================================================");
            ESP_LOGI(TAG, "Web Interface Available:");
            ESP_LOGI(TAG, "  URL: http://" IPSTR ":%d", IP2STR(&ip_info.ip), port);
            ESP_LOGI(TAG, "  IP Address: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI(TAG, "  Port: %d", port);
            ESP_LOGI(TAG, "=================================================");
        } else {
            ESP_LOGI(TAG, "Web server started on port %d (IP address not yet available)", port);
        }
    } else {
        ESP_LOGI(TAG, "Web server started on port %d (WiFi not connected)", port);
    }
}

// Server-Sent Events (SSE) client tracking
#define MAX_SSE_CLIENTS 4
static int sse_clients[MAX_SSE_CLIENTS];
static int sse_client_count = 0;

// SSE message types
typedef enum {
    SSE_MSG_RELAY_STATUS,
    SSE_MSG_SYSTEM_STATUS
} sse_message_type_t;

// Forward declarations for SSE functions
static void sse_add_client(int fd);
static void sse_remove_client(int fd);
static esp_err_t sse_send_to_all_clients(const char *event, const char *data);
static esp_err_t sse_handler(httpd_req_t *req);

// Helper function to mask sensitive configuration fields
static cJSON* mask_sensitive_config(const door_station_config_t *config) {
    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;
    
    cJSON_AddStringToObject(json, "wifi_ssid", config->wifi_ssid);
    cJSON_AddStringToObject(json, "wifi_password", strlen(config->wifi_password) > 0 ? "********" : "");
    cJSON_AddStringToObject(json, "sip_user", config->sip_user);
    cJSON_AddStringToObject(json, "sip_domain", config->sip_domain);
    cJSON_AddStringToObject(json, "sip_password", strlen(config->sip_password) > 0 ? "********" : "");
    cJSON_AddStringToObject(json, "sip_callee", config->sip_callee);
    cJSON_AddNumberToObject(json, "web_port", config->web_port);
    cJSON_AddNumberToObject(json, "door_pulse_duration", config->door_pulse_duration);
    
    return json;
}

// Helper function to parse configuration from JSON
static esp_err_t parse_config_from_json(const char *json_str, door_station_config_t *config) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Load current config first to preserve existing values
    esp_err_t ret = config_manager_load(config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load current config, using defaults");
        config_manager_get_defaults(config);
    }
    
    // Update fields that are present in JSON
    cJSON *item;
    
    item = cJSON_GetObjectItem(json, "wifi_ssid");
    if (cJSON_IsString(item)) {
        strncpy(config->wifi_ssid, item->valuestring, sizeof(config->wifi_ssid) - 1);
        config->wifi_ssid[sizeof(config->wifi_ssid) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "wifi_password");
    if (cJSON_IsString(item) && strcmp(item->valuestring, "********") != 0) {
        strncpy(config->wifi_password, item->valuestring, sizeof(config->wifi_password) - 1);
        config->wifi_password[sizeof(config->wifi_password) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "sip_user");
    if (cJSON_IsString(item)) {
        strncpy(config->sip_user, item->valuestring, sizeof(config->sip_user) - 1);
        config->sip_user[sizeof(config->sip_user) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "sip_domain");
    if (cJSON_IsString(item)) {
        strncpy(config->sip_domain, item->valuestring, sizeof(config->sip_domain) - 1);
        config->sip_domain[sizeof(config->sip_domain) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "sip_password");
    if (cJSON_IsString(item) && strcmp(item->valuestring, "********") != 0) {
        strncpy(config->sip_password, item->valuestring, sizeof(config->sip_password) - 1);
        config->sip_password[sizeof(config->sip_password) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "sip_callee");
    if (cJSON_IsString(item)) {
        strncpy(config->sip_callee, item->valuestring, sizeof(config->sip_callee) - 1);
        config->sip_callee[sizeof(config->sip_callee) - 1] = '\0';
    }
    
    item = cJSON_GetObjectItem(json, "web_port");
    if (cJSON_IsNumber(item)) {
        config->web_port = (uint16_t)item->valueint;
    }
    
    item = cJSON_GetObjectItem(json, "door_pulse_duration");
    if (cJSON_IsNumber(item)) {
        config->door_pulse_duration = (uint32_t)item->valueint;
    }
    
    cJSON_Delete(json);
    return ESP_OK;
}

// GET /api/config - Get current configuration
static esp_err_t config_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/config");
    
    door_station_config_t config;
    esp_err_t ret = config_manager_load(&config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config, using defaults");
        config_manager_get_defaults(&config);
    }
    
    cJSON *json = mask_sensitive_config(&config);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    return ESP_OK;
}

// POST /api/config - Update configuration
static esp_err_t config_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/config");
    
    // Read request body
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        } else {
            httpd_resp_send_500(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    // Parse configuration
    door_station_config_t config;
    esp_err_t parse_ret = parse_config_from_json(buf, &config);
    free(buf);
    
    if (parse_ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }
    
    // Validate configuration
    config_validation_error_t validation_result = config_manager_validate(&config);
    if (validation_result != CONFIG_VALIDATION_OK) {
        const char* error_msg = config_manager_get_validation_error_message(validation_result);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, error_msg);
        return ESP_FAIL;
    }
    
    // Save configuration
    esp_err_t save_ret = config_manager_save(&config);
    if (save_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(save_ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save configuration");
        return ESP_FAIL;
    }
    
    // Return success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", 19);
    
    ESP_LOGI(TAG, "Configuration updated successfully");
    return ESP_OK;
}

// POST /api/doorbell - Trigger virtual doorbell
static esp_err_t doorbell_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/doorbell - Virtual doorbell pressed");
    
    // Trigger button press event
    esp_err_t ret = io_events_publish_button(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to publish button press event: %s", esp_err_to_name(ret));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to trigger doorbell");
        return ESP_FAIL;
    }
    
    // Return success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Doorbell pressed\"}", 48);
    
    return ESP_OK;
}

// GET /api/relays - Get relay status
static esp_err_t relays_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/relays");
    
    // Get current relay states
    relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
    relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
    
    // Create JSON response
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON_AddBoolToObject(json, "door", door_state == RELAY_STATE_ON);
    cJSON_AddBoolToObject(json, "light", light_state == RELAY_STATE_ON);
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    return ESP_OK;
}

// GET /api/status - Get system status
static esp_err_t status_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/status");
    
    // Get relay states
    relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
    relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
    
    // Create JSON response
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // Add relay status
    cJSON *relays = cJSON_CreateObject();
    cJSON_AddBoolToObject(relays, "door", door_state == RELAY_STATE_ON);
    cJSON_AddBoolToObject(relays, "light", light_state == RELAY_STATE_ON);
    cJSON_AddItemToObject(json, "relays", relays);
    
    // Add system status
    cJSON_AddStringToObject(json, "system", "running");
    cJSON_AddBoolToObject(json, "web_server", true);
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    return ESP_OK;
}

// POST /api/factory-reset - Perform factory reset
static esp_err_t factory_reset_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/factory-reset - Factory reset requested");
    
    // Perform factory reset
    esp_err_t ret = config_manager_factory_reset();
    
    // Create JSON response
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    if (ret == ESP_OK) {
        cJSON_AddBoolToObject(json, "success", true);
        cJSON_AddStringToObject(json, "message", "Factory reset completed successfully");
        ESP_LOGI(TAG, "Factory reset completed successfully");
    } else {
        cJSON_AddBoolToObject(json, "success", false);
        cJSON_AddStringToObject(json, "message", "Factory reset failed");
        cJSON_AddStringToObject(json, "error", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Factory reset failed: %s", esp_err_to_name(ret));
    }
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    if (!json_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    
    if (ret == ESP_OK) {
        httpd_resp_set_status(req, "200 OK");
    } else {
        httpd_resp_set_status(req, "500 Internal Server Error");
    }
    
    httpd_resp_send(req, json_str, strlen(json_str));
    
    free(json_str);
    return ESP_OK;
}

// MIME type mapping
static const char* get_mime_type(const char* path) {
    const char* ext = strrchr(path, '.');
    if (!ext) return "text/plain";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    
    return "text/plain";
}

// Static file handler
static esp_err_t static_file_handler(httpd_req_t *req) {
    char filepath[512];
    const char* filename = req->uri;
    
    // Default to index.html for root path
    if (strcmp(filename, "/") == 0) {
        filename = "/index.html";
    }
    
    // Validate filename length to prevent buffer overflow
    // Reasonable limit for web file paths (buffer is 512, "/web_root" is 9)
    if (strlen(filename) > 200) {
        ESP_LOGW(TAG, "Filename too long: %s", filename);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Basic security check: prevent directory traversal
    if (strstr(filename, "..") != NULL) {
        ESP_LOGW(TAG, "Directory traversal attempt blocked: %s", filename);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Build full file path safely
    const char* prefix = "/web_root";
    size_t prefix_len = strlen(prefix);
    size_t filename_len = strlen(filename);
    
    // Double-check total length
    if (prefix_len + filename_len + 1 > sizeof(filepath)) {
        ESP_LOGW(TAG, "File path would be too long: %s", filename);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Safe string construction
    strcpy(filepath, prefix);
    strcat(filepath, filename);
    
    ESP_LOGI(TAG, "Serving file: %s", filepath);
    
    // Open file
    FILE* file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Set content type
    const char* mime_type = get_mime_type(filename);
    httpd_resp_set_type(req, mime_type);
    
    // Send file content
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            fclose(file);
            ESP_LOGE(TAG, "Failed to send file chunk");
            return ESP_FAIL;
        }
    }
    
    // Send empty chunk to signal end
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(file);
    
    return ESP_OK;
}

// Initialize SPIFFS for static file serving
static esp_err_t init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/web_root",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    
    return ESP_OK;
}

esp_err_t web_server_init(uint16_t port) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Initialize SPIFFS for static files
    esp_err_t ret = init_spiffs();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS");
        return ret;
    }
    
    // Configure HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    server_port = port;  // Store port for later use
    config.max_uri_handlers = 16;
    config.max_open_sockets = 7;
    config.stack_size = 8192;
    
    // Initialize SSE client tracking
    sse_client_count = 0;
    memset(sse_clients, 0, sizeof(sse_clients));
    
    // Start HTTP server
    ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register API endpoints first (more specific routes)
    httpd_uri_t config_get_uri = {
        .uri = "/api/config",
        .method = HTTP_GET,
        .handler = config_get_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &config_get_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register config GET handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    httpd_uri_t config_post_uri = {
        .uri = "/api/config",
        .method = HTTP_POST,
        .handler = config_post_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &config_post_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register config POST handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register doorbell API endpoint
    httpd_uri_t doorbell_post_uri = {
        .uri = "/api/doorbell",
        .method = HTTP_POST,
        .handler = doorbell_post_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &doorbell_post_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register doorbell POST handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register relays API endpoint
    httpd_uri_t relays_get_uri = {
        .uri = "/api/relays",
        .method = HTTP_GET,
        .handler = relays_get_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &relays_get_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register relays GET handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register status API endpoint
    httpd_uri_t status_get_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = status_get_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &status_get_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register status GET handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register Server-Sent Events endpoint
    httpd_uri_t sse_uri = {
        .uri = "/events",
        .method = HTTP_GET,
        .handler = sse_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &sse_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register SSE handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register factory reset API endpoint
    httpd_uri_t factory_reset_post_uri = {
        .uri = "/api/factory-reset",
        .method = HTTP_POST,
        .handler = factory_reset_post_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &factory_reset_post_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register factory reset POST handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Register static file handler (catch-all, must be last)
    httpd_uri_t static_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = static_file_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &static_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register static file handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Log web interface URL with IP address
    log_web_interface_url(port);
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (server == NULL) {
        ESP_LOGW(TAG, "Web server not running");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = httpd_stop(server);
    if (ret == ESP_OK) {
        server = NULL;
        server_port = 0;  // Clear stored port
        
        // Clear SSE client tracking
        sse_client_count = 0;
        memset(sse_clients, 0, sizeof(sse_clients));
        
        ESP_LOGI(TAG, "Web server stopped");
        
        // Unmount SPIFFS
        esp_vfs_spiffs_unregister(NULL);
    } else {
        ESP_LOGE(TAG, "Failed to stop web server: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

bool web_server_is_running(void) {
    return server != NULL;
}

// Server-Sent Events helper functions
static void sse_add_client(int fd) {
    if (sse_client_count < MAX_SSE_CLIENTS) {
        sse_clients[sse_client_count++] = fd;
        ESP_LOGI(TAG, "SSE client connected, fd=%d, total=%d", fd, sse_client_count);
    } else {
        ESP_LOGW(TAG, "Maximum SSE clients reached, rejecting fd=%d", fd);
    }
}

static void sse_remove_client(int fd) {
    for (int i = 0; i < sse_client_count; i++) {
        if (sse_clients[i] == fd) {
            // Shift remaining clients
            for (int j = i; j < sse_client_count - 1; j++) {
                sse_clients[j] = sse_clients[j + 1];
            }
            sse_client_count--;
            ESP_LOGI(TAG, "SSE client disconnected, fd=%d, total=%d", fd, sse_client_count);
            break;
        }
    }
}

static esp_err_t sse_send_to_all_clients(const char *event, const char *data) {
    if (sse_client_count == 0) {
        return ESP_OK; // No clients to send to
    }
    
    // Format SSE message
    char sse_message[1024];  // Increased buffer size
    int written = snprintf(sse_message, sizeof(sse_message), "event: %s\ndata: %s\n\n", event, data);
    
    // Check for truncation
    if (written >= sizeof(sse_message)) {
        ESP_LOGW(TAG, "SSE message truncated, event: %s", event);
        return ESP_ERR_NO_MEM;
    }
    
    // Send to all connected clients
    for (int i = sse_client_count - 1; i >= 0; i--) {
        esp_err_t ret = httpd_socket_send(server, sse_clients[i], sse_message, strlen(sse_message), 0);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to send SSE message to fd=%d: %s", sse_clients[i], esp_err_to_name(ret));
            // Remove failed client
            sse_remove_client(sse_clients[i]);
        }
    }
    
    return ESP_OK;
}

// Server-Sent Events handler
static esp_err_t sse_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "SSE connection established");
    
    // Set SSE headers
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    // Add client to tracking
    int client_fd = httpd_req_to_sockfd(req);
    sse_add_client(client_fd);
    
    // Send initial connection message
    const char *welcome_msg = "event: connected\ndata: {\"status\":\"connected\"}\n\n";
    esp_err_t ret = httpd_resp_send_chunk(req, welcome_msg, strlen(welcome_msg));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send SSE welcome message");
        sse_remove_client(client_fd);
        return ret;
    }
    
    // Send current relay status
    relay_state_t door_state = io_manager_get_relay_state(RELAY_DOOR);
    relay_state_t light_state = io_manager_get_relay_state(RELAY_LIGHT);
    
    cJSON *json = cJSON_CreateObject();
    if (json) {
        cJSON_AddStringToObject(json, "type", "relay_status");
        cJSON *data = cJSON_CreateObject();
        cJSON_AddBoolToObject(data, "door", door_state == RELAY_STATE_ON);
        cJSON_AddBoolToObject(data, "light", light_state == RELAY_STATE_ON);
        cJSON_AddItemToObject(json, "data", data);
        
        char *json_str = cJSON_Print(json);
        if (json_str) {
            char sse_msg[512];  // Increased buffer size
            int written = snprintf(sse_msg, sizeof(sse_msg), "event: relay_status\ndata: %s\n\n", json_str);
            
            if (written < sizeof(sse_msg)) {
                httpd_resp_send_chunk(req, sse_msg, strlen(sse_msg));
            } else {
                ESP_LOGW(TAG, "SSE relay status message truncated");
            }
            free(json_str);
        }
        cJSON_Delete(json);
    }
    
    // Keep connection alive - this will block until client disconnects
    // In a real implementation, you might want to handle this differently
    httpd_resp_send_chunk(req, NULL, 0); // End chunked response
    
    return ESP_OK;
}

// Public function to broadcast relay status updates
esp_err_t web_server_broadcast_relay_status(relay_id_t relay, relay_state_t state) {
    if (!web_server_is_running() || sse_client_count == 0) {
        return ESP_OK; // No server or clients
    }
    
    cJSON *json = cJSON_CreateObject();
    if (!json) return ESP_ERR_NO_MEM;
    
    cJSON_AddStringToObject(json, "type", "relay_status");
    
    cJSON *data = cJSON_CreateObject();
    if (relay == RELAY_DOOR) {
        cJSON_AddBoolToObject(data, "door", state == RELAY_STATE_ON);
        cJSON_AddBoolToObject(data, "light", io_manager_get_relay_state(RELAY_LIGHT) == RELAY_STATE_ON);
    } else {
        cJSON_AddBoolToObject(data, "door", io_manager_get_relay_state(RELAY_DOOR) == RELAY_STATE_ON);
        cJSON_AddBoolToObject(data, "light", state == RELAY_STATE_ON);
    }
    cJSON_AddItemToObject(json, "data", data);
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    if (!json_str) return ESP_ERR_NO_MEM;
    
    esp_err_t ret = sse_send_to_all_clients("relay_status", json_str);
    free(json_str);
    
    return ret;
}

// Public function to log web interface URL
esp_err_t web_server_log_url(void) {
    if (!web_server_is_running()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    log_web_interface_url(server_port);
    return ESP_OK;
}