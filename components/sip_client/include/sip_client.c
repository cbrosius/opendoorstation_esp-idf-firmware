#include "sip_client.h"
#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include <lwip/sockets.h>
#include <mbedtls/md5.h>

static const char* TAG = "SIP_CLIENT";

// Generate unique identifiers
static void generate_call_id(sip_client_t* client, char* call_id, size_t max_len) {
    snprintf(call_id, max_len, "%08x-%08x@%s", 
             client->call_id_counter++, esp_random(), client->local_ip);
}

static void generate_branch_id(char* branch, size_t max_len) {
    snprintf(branch, max_len, "z9hG4bK%08x%08x", esp_random(), esp_random());
}

static void generate_tag(char* tag, size_t max_len) {
    snprintf(tag, max_len, "tag%08x", esp_random());
}

// MD5 Hash for Digest Authentication
static void calculate_md5_hash(const char* input, char* output) {
    unsigned char hash[16];
    mbedtls_md5_context ctx;
    
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts_ret(&ctx);
    mbedtls_md5_update_ret(&ctx, (const unsigned char*)input, strlen(input));
    mbedtls_md5_finish_ret(&ctx, hash);
    mbedtls_md5_free(&ctx);
    
    for (int i = 0; i < 16; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[32] = '\0';
}

// Create REGISTER message
static int create_register_message(sip_client_t* client, char* buffer, size_t max_len, bool with_auth) {
    char call_id[64], branch[32], from_tag[32];
    
    generate_call_id(client, call_id, sizeof(call_id));
    generate_branch_id(branch, sizeof(branch));
    generate_tag(from_tag, sizeof(from_tag));
    
    int written = snprintf(buffer, max_len,
        "REGISTER sip:%s SIP/2.0\r\n"
        "Via: SIP/2.0/UDP %s:%d;branch=%s\r\n"
        "Max-Forwards: 70\r\n"
        "From: \"%s\" <sip:%s@%s>;tag=%s\r\n"
        "To: \"%s\" <sip:%s@%s>\r\n"
        "Call-ID: %s\r\n"
        "CSeq: %d REGISTER\r\n"
        "Contact: <sip:%s@%s:%d>\r\n"
        "Expires: %d\r\n"
        "User-Agent: ESP32-DoorStation/1.0\r\n",
        
        client->domain,
        client->local_ip, client->local_port, branch,
        client->display_name, client->username, client->domain, from_tag,
        client->display_name, client->username, client->domain,
        call_id,
        client->cseq_counter++,
        client->username, client->local_ip, client->local_port,
        client->register_expires
    );
    
    if (with_auth && client->auth.auth_required) {
        char ha1[33], ha2[33], response[33];
        char ha1_input[256], ha2_input[256], response_input[512];
        
        // HA1 = MD5(username:realm:password)
        snprintf(ha1_input, sizeof(ha1_input), "%s:%s:%s",
                client->username, client->auth.realm, client->password);
        calculate_md5_hash(ha1_input, ha1);
        
        // HA2 = MD5(method:uri)
        snprintf(ha2_input, sizeof(ha2_input), "REGISTER:sip:%s", client->domain);
        calculate_md5_hash(ha2_input, ha2);
        
        // Response = MD5(HA1:nonce:HA2)
        snprintf(response_input, sizeof(response_input), "%s:%s:%s",
                ha1, client->auth.nonce, ha2);
        calculate_md5_hash(response_input, response);
        
        written += snprintf(buffer + written, max_len - written,
            "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", "
            "uri=\"sip:%s\", response=\"%s\", algorithm=%s\r\n",
            client->username, client->auth.realm, client->auth.nonce,
            client->domain, response, client->auth.algorithm);
    }
    
    written += snprintf(buffer + written, max_len - written, "Content-Length: 0\r\n\r\n");
    return written;
}

// Parse WWW-Authenticate header for 401 responses
static void parse_www_authenticate(sip_client_t* client, const char* auth_header) {
    char* realm_start = strstr(auth_header, "realm=\"");
    char* nonce_start = strstr(auth_header, "nonce=\"");
    char* algorithm_start = strstr(auth_header, "algorithm=");
    char* qop_start = strstr(auth_header, "qop=\"");
    
    if (realm_start) {
        realm_start += 7; // Skip 'realm="'
        char* realm_end = strchr(realm_start, '"');
        if (realm_end) {
            int len = realm_end - realm_start;
            strncpy(client->auth.realm, realm_start, len);
            client->auth.realm[len] = '\0';
        }
    }
    
    if (nonce_start) {
        nonce_start += 7; // Skip 'nonce="'
        char* nonce_end = strchr(nonce_start, '"');
        if (nonce_end) {
            int len = nonce_end - nonce_start;
            strncpy(client->auth.nonce, nonce_start, len);
            client->auth.nonce[len] = '\0';
        }
    }
    
    if (algorithm_start) {
        algorithm_start += 10; // Skip 'algorithm='
        char* algo_end = strpbrk(algorithm_start, " ,\r\n");
        if (algo_end) {
            int len = algo_end - algorithm_start;
            strncpy(client->auth.algorithm, algorithm_start, len);
            client->auth.algorithm[len] = '\0';
        } else {
            strcpy(client->auth.algorithm, "MD5");
        }
    }
    
    client->auth.auth_required = true;
}

// Core implementation functions
esp_err_t sip_client_init(sip_client_t* client) {
    if (!client) return ESP_ERR_INVALID_ARG;
    
    memset(client, 0, sizeof(sip_client_t));
    
    // Default values
    client->server_port = SIP_DEFAULT_PORT;
    client->local_port = SIP_DEFAULT_PORT;
    client->socket_fd = -1;
    client->state = SIP_STATE_IDLE;
    client->call_id_counter = esp_random();
    client->cseq_counter = 1;
    client->register_expires = 3600;
    
    strcpy(client->auth.algorithm, "MD5");
    
    ESP_LOGI(TAG, "SIP Client initialized");
    return ESP_OK;
}

esp_err_t sip_client_start(sip_client_t* client) {
    if (!client) return ESP_ERR_INVALID_ARG;
    
    // Create UDP socket
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client->socket_fd < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    
    // Set socket options
    int reuse = 1;
    setsockopt(client->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // Bind local socket
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(client->local_port);
    
    if (bind(client->socket_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
        close(client->socket_fd);
        client->socket_fd = -1;
        return ESP_FAIL;
    }
    
    // Setup server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->server_port);
    inet_pton(AF_INET, client->server_ip, &client->server_addr.sin_addr);
    
    ESP_LOGI(TAG, "SIP Client started on port %d", client->local_port);
    return ESP_OK;
}

esp_err_t sip_client_register(sip_client_t* client) {
    if (!client || client->socket_fd < 0) return ESP_ERR_INVALID_STATE;
    
    char register_msg[SIP_MAX_MESSAGE_SIZE];
    int msg_len = create_register_message(client, register_msg, sizeof(register_msg), false);
    
    int sent = sendto(client->socket_fd, register_msg, msg_len, 0,
                      (struct sockaddr*)&client->server_addr, sizeof(client->server_addr));
    
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send REGISTER: errno %d", errno);
        return ESP_FAIL;
    }
    
    client->state = SIP_STATE_REGISTERING;
    ESP_LOGI(TAG, "REGISTER sent (%d bytes)", sent);
    ESP_LOGD(TAG, "REGISTER message:\n%s", register_msg);
    
    return ESP_OK;
}

esp_err_t sip_client_process_message(sip_client_t* client, char* buffer, int len, struct sockaddr_in* from) {
    if (!client || !buffer || len <= 0) return ESP_ERR_INVALID_ARG;
    
    ESP_LOGI(TAG, "Processing SIP message (%d bytes)", len);
    ESP_LOGD(TAG, "Message:\n%.*s", len, buffer);
    
    // Process responses (SIP/2.0)
    if (strncmp(buffer, "SIP/2.0", 7) == 0) {
        if (strncmp(buffer + 8, "200", 3) == 0) {
            // 200 OK Response
            if (client->state == SIP_STATE_REGISTERING) {
                client->state = SIP_STATE_REGISTERED;
                ESP_LOGI(TAG, "Registration successful");
                if (client->on_registration_success) {
                    client->on_registration_success();
                }
            }
        } else if (strncmp(buffer + 8, "401", 3) == 0) {
            // 401 Unauthorized - Extract authentication info
            char* auth_header = strstr(buffer, "WWW-Authenticate:");
            if (auth_header) {
                parse_www_authenticate(client, auth_header);
                
                // Retry REGISTER with authentication
                char register_msg[SIP_MAX_MESSAGE_SIZE];
                int msg_len = create_register_message(client, register_msg, sizeof(register_msg), true);
                
                int sent = sendto(client->socket_fd, register_msg, msg_len, 0,
                                 (struct sockaddr*)&client->server_addr, sizeof(client->server_addr));
                
                if (sent > 0) {
                    ESP_LOGI(TAG, "REGISTER with auth sent");
                } else {
                    ESP_LOGE(TAG, "Failed to send authenticated REGISTER");
                }
            }
        }
    } 
    // Process requests (INVITE, BYE, etc.)
    else if (strncmp(buffer, "INVITE", 6) == 0) {
        ESP_LOGI(TAG, "Incoming INVITE received");
        
        // Extract caller information
        char* from_header = strstr(buffer, "From:");
        if (from_header) {
            // Simple caller ID extraction (can be improved)
            char caller_id[64] = "Unknown";
            if (client->on_incoming_call) {
                client->on_incoming_call(caller_id);
            }
        }
        
        // Send 180 Ringing
        sip_client_send_response(client, 180, "Ringing", client->call_info.call_id);
    }
    
    return ESP_OK;
}

esp_err_t sip_client_send_response(sip_client_t* client, int status_code, const char* reason, const char* call_id) {
    // Simplified response sending - can be extended
    char response[512];
    int len = snprintf(response, sizeof(response),
        "SIP/2.0 %d %s\r\n"
        "Content-Length: 0\r\n\r\n",
        status_code, reason);
    
    int sent = sendto(client->socket_fd, response, len, 0,
                     (struct sockaddr*)&client->server_addr, sizeof(client->server_addr));
    
    return (sent > 0) ? ESP_OK : ESP_FAIL;
}

void sip_client_set_callbacks(sip_client_t* client,
                             void (*on_reg_success)(void),
                             void (*on_reg_failed)(void),
                             void (*on_incoming_call)(const char*),
                             void (*on_call_ended)(void),
                             void (*on_dtmf)(char)) {
    if (!client) return;
    
    client->on_registration_success = on_reg_success;
    client->on_registration_failed = on_reg_failed;
    client->on_incoming_call = on_incoming_call;
    client->on_call_ended = on_call_ended;
    client->on_dtmf_received = on_dtmf;
}

const char* sip_state_to_string(sip_state_t state) {
    switch (state) {
        case SIP_STATE_IDLE: return "IDLE";
        case SIP_STATE_REGISTERING: return "REGISTERING";
        case SIP_STATE_REGISTERED: return "REGISTERED";
        case SIP_STATE_CALLING: return "CALLING";
        case SIP_STATE_IN_CALL: return "IN_CALL";
        case SIP_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void sip_client_stop(sip_client_t* client) {
    if (client && client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
        client->state = SIP_STATE_IDLE;
        ESP_LOGI(TAG, "SIP Client stopped");
    }
}
