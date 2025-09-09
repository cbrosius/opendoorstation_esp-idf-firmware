#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <inttypes.h>

static const char *TAG = "wifi_manager";

// WiFi configuration
#define WIFI_MAXIMUM_RETRY 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Global state
static wifi_info_t g_wifi_info = {0};
static SemaphoreHandle_t g_wifi_mutex = NULL;
static bool g_wifi_initialized = false;
static wifi_event_callback_t g_event_callback = NULL;
static void *g_callback_user_data = NULL;
static esp_netif_t *g_sta_netif = NULL;

// Forward declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data);
static void ip_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
static void update_wifi_info(wifi_state_t state);
static void notify_callback(void);

esp_err_t wifi_manager_init(void)
{
    if (g_wifi_initialized) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi manager");

    // Create mutex for thread safety
    g_wifi_mutex = xSemaphoreCreateMutex();
    if (g_wifi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize TCP/IP stack
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP stack: %s", esp_err_to_name(ret));
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    // Create default WiFi station
    g_sta_netif = esp_netif_create_default_wifi_sta();
    if (g_sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi station");
        vSemaphoreDelete(g_wifi_mutex);
        return ESP_FAIL;
    }

    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    // Register event handlers
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                     &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        esp_wifi_deinit();
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                     &ip_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_wifi_deinit();
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    // Set WiFi mode to station
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler);
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_wifi_deinit();
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler);
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
        esp_wifi_deinit();
        vSemaphoreDelete(g_wifi_mutex);
        return ret;
    }

    // Initialize WiFi info
    memset(&g_wifi_info, 0, sizeof(wifi_info_t));
    g_wifi_info.state = WIFI_STATE_DISCONNECTED;

    g_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi manager initialized successfully");

    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
    if (!g_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to WiFi network: %s", ssid);

    // Configure WiFi connection
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL && strlen(password) > 0) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi configuration: %s", esp_err_to_name(ret));
        return ret;
    }

    // Update state and start connection
    update_wifi_info(WIFI_STATE_CONNECTING);
    strncpy(g_wifi_info.ssid, ssid, sizeof(g_wifi_info.ssid) - 1);

    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi connection: %s", esp_err_to_name(ret));
        update_wifi_info(WIFI_STATE_ERROR);
        return ret;
    }

    return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
    if (!g_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Disconnecting from WiFi");

    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    update_wifi_info(WIFI_STATE_DISCONNECTED);
    return ESP_OK;
}

esp_err_t wifi_manager_get_info(wifi_info_t *info)
{
    if (!g_wifi_initialized || info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(info, &g_wifi_info, sizeof(wifi_info_t));
        xSemaphoreGive(g_wifi_mutex);
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_manager_register_callback(wifi_event_callback_t callback, void *user_data)
{
    if (!g_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    g_event_callback = callback;
    g_callback_user_data = user_data;
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    if (!g_wifi_initialized) {
        return false;
    }

    if (xSemaphoreTake(g_wifi_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        bool connected = (g_wifi_info.state == WIFI_STATE_CONNECTED);
        xSemaphoreGive(g_wifi_mutex);
        return connected;
    }

    return false;
}

const char* wifi_manager_get_ip_address(void)
{
    if (!g_wifi_initialized || !wifi_manager_is_connected()) {
        return NULL;
    }

    return g_wifi_info.ip_address;
}

const char* wifi_manager_get_state_string(wifi_state_t state)
{
    switch (state) {
        case WIFI_STATE_DISCONNECTED: return "DISCONNECTED";
        case WIFI_STATE_CONNECTING: return "CONNECTING";
        case WIFI_STATE_CONNECTED: return "CONNECTED";
        case WIFI_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

esp_err_t wifi_manager_stop(void)
{
    if (!g_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Stopping WiFi manager");

    // Disconnect and stop WiFi
    esp_wifi_disconnect();
    esp_wifi_stop();

    // Unregister event handlers
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);

    // Cleanup WiFi
    esp_wifi_deinit();

    // Cleanup resources
    if (g_wifi_mutex != NULL) {
        vSemaphoreDelete(g_wifi_mutex);
        g_wifi_mutex = NULL;
    }

    g_wifi_initialized = false;
    ESP_LOGI(TAG, "WiFi manager stopped");

    return ESP_OK;
}

// Private functions

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;

            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
                ESP_LOGI(TAG, "Connected to WiFi network: %s (channel %d)", 
                         event->ssid, event->channel);
                break;
            }

            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
                ESP_LOGW(TAG, "Disconnected from WiFi network (reason: %d)", event->reason);
                
                if (xSemaphoreTake(g_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    g_wifi_info.retry_count++;
                    
                    if (g_wifi_info.retry_count < WIFI_MAXIMUM_RETRY) {
                        ESP_LOGI(TAG, "Retrying WiFi connection (%" PRIu32 "/%d)", 
                                 g_wifi_info.retry_count, WIFI_MAXIMUM_RETRY);
                        esp_wifi_connect();
                        update_wifi_info(WIFI_STATE_CONNECTING);
                    } else {
                        ESP_LOGE(TAG, "WiFi connection failed after %d retries", WIFI_MAXIMUM_RETRY);
                        update_wifi_info(WIFI_STATE_ERROR);
                    }
                    
                    xSemaphoreGive(g_wifi_mutex);
                }
                break;
            }

            default:
                break;
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        // Convert IP address to string
        char ip_str[16];
        esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
        
        char gw_str[16];
        esp_ip4addr_ntoa(&event->ip_info.gw, gw_str, sizeof(gw_str));
        
        char netmask_str[16];
        esp_ip4addr_ntoa(&event->ip_info.netmask, netmask_str, sizeof(netmask_str));

        ESP_LOGI(TAG, "WiFi connected successfully!");
        ESP_LOGI(TAG, "IP Address: %s", ip_str);
        ESP_LOGI(TAG, "Gateway: %s", gw_str);
        ESP_LOGI(TAG, "Netmask: %s", netmask_str);

        if (xSemaphoreTake(g_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            strncpy(g_wifi_info.ip_address, ip_str, sizeof(g_wifi_info.ip_address) - 1);
            strncpy(g_wifi_info.gateway, gw_str, sizeof(g_wifi_info.gateway) - 1);
            strncpy(g_wifi_info.netmask, netmask_str, sizeof(g_wifi_info.netmask) - 1);
            g_wifi_info.connect_time = esp_timer_get_time() / 1000000;
            g_wifi_info.retry_count = 0;
            
            // Get RSSI
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                g_wifi_info.rssi = ap_info.rssi;
                ESP_LOGI(TAG, "Signal strength: %d dBm", g_wifi_info.rssi);
            }
            
            update_wifi_info(WIFI_STATE_CONNECTED);
            xSemaphoreGive(g_wifi_mutex);
        }
    }
}

static void update_wifi_info(wifi_state_t state)
{
    if (xSemaphoreTake(g_wifi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        wifi_state_t old_state = g_wifi_info.state;
        g_wifi_info.state = state;
        
        if (state == WIFI_STATE_DISCONNECTED || state == WIFI_STATE_ERROR) {
            memset(g_wifi_info.ip_address, 0, sizeof(g_wifi_info.ip_address));
            memset(g_wifi_info.gateway, 0, sizeof(g_wifi_info.gateway));
            memset(g_wifi_info.netmask, 0, sizeof(g_wifi_info.netmask));
            g_wifi_info.rssi = 0;
        }
        
        xSemaphoreGive(g_wifi_mutex);
        
        if (old_state != state) {
            ESP_LOGI(TAG, "WiFi state changed: %s -> %s", 
                     wifi_manager_get_state_string(old_state),
                     wifi_manager_get_state_string(state));
            notify_callback();
        }
    }
}

static void notify_callback(void)
{
    if (g_event_callback != NULL) {
        g_event_callback(g_wifi_info.state, &g_wifi_info, g_callback_user_data);
    }
}