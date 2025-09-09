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
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

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

static const char* get_content_type(const char* file_path) {
    const char* ext = strrchr(file_path, '.');
    if (!ext) return "application/octet-stream";
    ext++; // Skip the dot
    
    if (strcasecmp(ext, "html") == 0) return "text/html";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "jpg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "ico") == 0) return "image/x-icon";
    
    return "application/octet-stream";
}

static esp_err_t static_get_handler(httpd_req_t *req)
{
    char file_path[256] = {0};  // Buffer for file path
    char test_path[256] = {0};  // Buffer for testing paths
    const char* uri_path = req->uri;
    const size_t max_path = sizeof(file_path) - 1;  // Leave room for null terminator
    
    ESP_LOGI(TAG, "=== Static File Request ===");
    ESP_LOGI(TAG, "URI: '%s'", uri_path);
    ESP_LOGI(TAG, "Method: %d", req->method);
    
    // Debug: List all files in SPIFFS
    DIR* dir = opendir("/web_root");
    if (dir) {
        struct dirent* ent;
        ESP_LOGI(TAG, "Files in SPIFFS:");
        while ((ent = readdir(dir)) != NULL) {
            ESP_LOGI(TAG, "  - %s", ent->d_name);
        }
        closedir(dir);
    } else {
        ESP_LOGW(TAG, "Could not open SPIFFS directory");
    }
    
    // Basic security check: prevent directory traversal
    if (strstr(uri_path, "..") != NULL) {
        ESP_LOGW(TAG, "Directory traversal attempt blocked: %s", uri_path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    
    // Initialize file path
    file_path[0] = '\0';
    memset(file_path, 0, sizeof(file_path));  // Ensure buffer is clean
    
    // Handle root path
    if (strlen(uri_path) == 0 || strcmp(uri_path, "/") == 0) {
        ESP_LOGI(TAG, "Root path requested, serving index.html");
        strncpy(file_path, "/web_root/index.html", max_path);
        file_path[max_path] = '\0';
    } else {
        // Skip leading slash for path comparison
        const char* path = uri_path;
        if (path[0] == '/') {
            path++;
        }

        // If the client already requested a path starting with "web_root/",
        // use it directly to avoid creating "/web_root/web_root/...".
        if (strncmp(path, "web_root/", 8) == 0) {
            // path already contains the web_root prefix
            if (strlen(path) + 1 <= sizeof(file_path)) {
                // Prepend a leading slash to make an absolute path
                file_path[0] = '/';
                strncpy(file_path + 1, path, sizeof(file_path) - 2);
                file_path[sizeof(file_path) - 1] = '\0';
            } else {
                ESP_LOGE(TAG, "Path too long: %s", path);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        } else {
            // Safely construct the file path with the web_root prefix
            size_t needed = strlen(path) + strlen("/web_root/") + 1; // +1 for null
            if (needed <= sizeof(file_path)) {
                // Build path safely without formatted output to silence compiler truncation warnings
                strncpy(file_path, "/web_root/", sizeof(file_path));
                file_path[sizeof(file_path) - 1] = '\0';
                strncat(file_path, path, sizeof(file_path) - strlen(file_path) - 1);
            } else {
                ESP_LOGE(TAG, "Path too long: %s", path);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
    }
    
    ESP_LOGI(TAG, "Looking for file: '%s'", file_path);

    // Check if file exists
    struct stat st;
    if (stat(file_path, &st) != 0) {
        ESP_LOGW(TAG, "File not found: '%s' (errno: %d)", file_path, errno);
        
        // Try alternative paths for the file
        const char* path = uri_path;
        if (path[0] == '/') path++; // Skip leading slash if present
        
        // Try all possible locations
        bool found = false;
        if (strlen(path) <= max_path - 20) { // Leave room for prefixes/extensions
            const char* test_patterns[] = {
                "/web_root/%s",          // Try direct path in web_root
                "/web_root/static/%s",   // Try static subdirectory
                "/web_root/css/%s",      // Try css subdirectory
                "/web_root/js/%s"        // Try js subdirectory
            };
            
            for (size_t i = 0; i < sizeof(test_patterns)/sizeof(test_patterns[0]); i++) {
                snprintf(test_path, sizeof(test_path), test_patterns[i], path);
                if (stat(test_path, &st) == 0) {
                    strncpy(file_path, test_path, max_path);
                    file_path[max_path] = '\0';
                    found = true;
                    ESP_LOGI(TAG, "Found file at alternate location: '%s'", file_path);
                    break;
                }
            }
            
            // If still not found and no extension provided, try with common extensions
            if (!found && strchr(path, '.') == NULL) {
                const char* extensions[] = {".html", ".htm", ".txt"};
                for (size_t i = 0; i < sizeof(extensions)/sizeof(extensions[0]); i++) {
                    if (snprintf(test_path, sizeof(test_path), "/web_root/%s%s", path, extensions[i]) < (int)sizeof(test_path)) {
                        if (stat(test_path, &st) == 0) {
                            strncpy(file_path, test_path, max_path);
                            file_path[max_path] = '\0';
                            ESP_LOGI(TAG, "Found file with extension: '%s'", file_path);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Open the file
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file: '%s' (errno: %d - %s)", file_path, errno, strerror(errno));
        
        // Extract the filename for logging
        const char* filename = strrchr(file_path, '/');
        filename = filename ? filename + 1 : file_path;
        
        // Log what we're looking for and what's available
        ESP_LOGI(TAG, "Looking for file '%s' in SPIFFS", filename);
        DIR* dir = opendir("/web_root");
        if (dir) {
            struct dirent* ent;
            ESP_LOGI(TAG, "Available files:");
            while ((ent = readdir(dir)) != NULL) {
                ESP_LOGI(TAG, "  - '%s'", ent->d_name);
            }
            closedir(dir);
        }
        
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // Get latest file stats for sending
    if (fstat(fileno(fp), &st) != 0) {
        ESP_LOGE(TAG, "Failed to get file size: %s", strerror(errno));
        fclose(fp);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Set content type and content length
    const char* content_type = get_content_type(file_path);
    httpd_resp_set_type(req, content_type);
    
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%lld", (long long)st.st_size);
    httpd_resp_set_hdr(req, "Content-Length", content_length);

    ESP_LOGI(TAG, "Serving file '%s' (%lld bytes)", file_path, (long long)st.st_size);
    
    // Read and send file in chunks
    char chunk[1024];
    size_t remaining = (size_t)st.st_size;
    
    while (remaining > 0) {
        size_t chunk_size = (remaining < sizeof(chunk)) ? remaining : sizeof(chunk);
        size_t read_bytes = fread(chunk, 1, chunk_size, fp);
        
        if (read_bytes != chunk_size) {
            ESP_LOGE(TAG, "Failed to read file: %s", strerror(errno));
            fclose(fp);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send chunk!");
            fclose(fp);
            httpd_resp_send_chunk(req, NULL, 0);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        remaining -= read_bytes;
    }
    
    fclose(fp);
    httpd_resp_send_chunk(req, NULL, 0);
    ESP_LOGI(TAG, "File '%s' sent successfully", file_path);
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



// Initialize SPIFFS for static file serving
static esp_err_t init_spiffs(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/web_root",  // This must match the path we use in static_get_handler
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = false  // Don't format if mount fails, we want to see the error
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
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SPIFFS mounted successfully. Total: %d bytes, Used: %d bytes", total, used);
    
    // List all files in SPIFFS
    DIR* dir = opendir("/web_root");
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory /web_root (errno: %d - %s)", errno, strerror(errno));
        return ESP_FAIL;
    }
    
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        struct stat st;
        char fullpath[600];
        snprintf(fullpath, sizeof(fullpath), "/web_root/%s", ent->d_name);
        if (stat(fullpath, &st) == 0) {
            ESP_LOGI(TAG, "Found file: %s (%ld bytes)", ent->d_name, st.st_size);
        }
    }
    closedir(dir);
    
    // Specifically check for index.html
    struct stat st;
    if (stat("/web_root/index.html", &st) == 0) {
        ESP_LOGI(TAG, "index.html found (%ld bytes)", st.st_size);
    } else {
        ESP_LOGE(TAG, "index.html not found! (errno: %d - %s)", errno, strerror(errno));
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGI(TAG, "Attempting to format SPIFFS partition...");
            ret = esp_spiffs_format(conf.partition_label);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to format SPIFFS: %s", esp_err_to_name(ret));
                return ret;
            }
            ESP_LOGI(TAG, "SPIFFS formatted successfully");
            return ESP_OK;
        }
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SPIFFS initialization complete. Total: %d bytes, Used: %d bytes", total, used);
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
    
    // Register handlers in order of specificity: most specific first
    
    // 1. Register specific API endpoints
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

    // 2. Register root path handler explicitly
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    ret = httpd_register_uri_handler(server, &root_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register root handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }

    // 3. Register catch-all handler for other static files
    httpd_uri_t static_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    
    ret = httpd_register_uri_handler(server, &static_get_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register status GET handler: %s", esp_err_to_name(ret));
        httpd_stop(server);
        server = NULL;
        return ret;
    }
    
    // Also register handlers for commonly requested literal paths that may appear
    // in requests (some clients request full paths like /web_root/index.html)
    httpd_uri_t webroot_prefix_uri = {
        .uri = "/web_root/*",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    ret = httpd_register_uri_handler(server, &webroot_prefix_uri);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Could not register /web_root/* handler: %s", esp_err_to_name(ret));
    }

    // Register specific asset paths as well to be safe
    httpd_uri_t index_uri = {
        .uri = "/index.html",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    if (httpd_register_uri_handler(server, &index_uri) != ESP_OK) {
        ESP_LOGW(TAG, "Could not register /index.html handler");
    }

    httpd_uri_t css_uri = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    if (httpd_register_uri_handler(server, &css_uri) != ESP_OK) {
        ESP_LOGW(TAG, "Could not register /style.css handler");
    }

    httpd_uri_t js_uri = {
        .uri = "/app.js",
        .method = HTTP_GET,
        .handler = static_get_handler,
        .user_ctx = NULL
    };
    if (httpd_register_uri_handler(server, &js_uri) != ESP_OK) {
        ESP_LOGW(TAG, "Could not register /app.js handler");
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
