#include "sip_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char* TAG = "SIP_INTEGRATION";
static sip_client_t sip_client;
static TaskHandle_t sip_task_handle = NULL;

// Callback Functions
static void on_registration_success(void) {
    ESP_LOGI(TAG, "✅ SIP Registration successful!");
}

static void on_registration_failed(void) {
    ESP_LOGW(TAG, "❌ SIP Registration failed!");
}

static void on_incoming_call(const char* caller_id) {
    ESP_LOGI(TAG, "📞 Incoming call from: %s", caller_id);
    // Auto-answer after 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    sip_client_answer_call(&sip_client);
}

static void on_call_ended(void) {
    ESP_LOGI(TAG, "📴 Call ended");
}

static void on_dtmf_received(char dtmf_code) {
    ESP_LOGI(TAG, "🎵 DTMF received: %c", dtmf_code);
    
    // Door opening logic
    if (dtmf_code == '#' || dtmf_code == '*') {
        ESP_LOGI(TAG, "🚪 Opening door!");
        // TODO: Implement door relay control
    }
}

// SIP Processing Task
static void sip_processing_task(void* params) {
    char buffer[SIP_MAX_MESSAGE_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ESP_LOGI(TAG, "SIP processing task started");
    
    while (1) {
        int len = recvfrom(sip_client.socket_fd, buffer, sizeof(buffer) - 1, 0,
                          (struct sockaddr*)&from_addr, &from_len);
        
        if (len > 0) {
            buffer[len] = '\0';
            sip_client_process_message(&sip_client, buffer, len, &from_addr);
        } else if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            ESP_LOGE(TAG, "SIP socket error: %d", errno);
            break;
        }
        
        // Re-registration every 30 minutes
        static uint32_t last_register = 0;
        if ((xTaskGetTickCount() - last_register) > pdMS_TO_TICKS(1800000)) {
            if (sip_client.state == SIP_STATE_REGISTERED) {
                sip_client_register(&sip_client);
                last_register = xTaskGetTickCount();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    sip_task_handle = NULL;
    vTaskDelete(NULL);
}

// Public API
esp_err_t sip_integration_init(const char* server_ip, const char* username, 
                              const char* password, const char* local_ip) {
    
    // Initialize SIP client
    esp_err_t ret = sip_client_init(&sip_client);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Configure SIP client
    strncpy(sip_client.server_ip, server_ip, sizeof(sip_client.server_ip) - 1);
    strncpy(sip_client.username, username, sizeof(sip_client.username) - 1);
    strncpy(sip_client.password, password, sizeof(sip_client.password) - 1);
    strncpy(sip_client.local_ip, local_ip, sizeof(sip_client.local_ip) - 1);
    strncpy(sip_client.domain, server_ip, sizeof(sip_client.domain) - 1);
    strncpy(sip_client.display_name, "Door Station", sizeof(sip_client.display_name) - 1);
    
    // Set callbacks
    sip_client_set_callbacks(&sip_client,
                            on_registration_success,
                            on_registration_failed,
                            on_incoming_call,
                            on_call_ended,
                            on_dtmf_received);
    
    return ESP_OK;
}

esp_err_t sip_integration_start(void) {
    // Start SIP client
    esp_err_t ret = sip_client_start(&sip_client);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Create SIP processing task
    xTaskCreate(sip_processing_task, "sip_task", 8192, NULL, 5, &sip_task_handle);
    
    // Wait a bit then register
    vTaskDelay(pdMS_TO_TICKS(1000));
    ret = sip_client_register(&sip_client);
    
    return ret;
}

void sip_integration_stop(void) {
    if (sip_task_handle) {
        vTaskDelete(sip_task_handle);
        sip_task_handle = NULL;
    }
    sip_client_stop(&sip_client);
}

sip_state_t sip_integration_get_state(void) {
    return sip_client.state;
}
