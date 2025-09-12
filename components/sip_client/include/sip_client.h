#ifndef SIP_CLIENT_H
#define SIP_CLIENT_H

#include <esp_err.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdint.h>

#define SIP_MAX_MESSAGE_SIZE 2048
#define SIP_MAX_USERNAME_LEN 32
#define SIP_MAX_PASSWORD_LEN 32
#define SIP_MAX_IP_LEN 16
#define SIP_DEFAULT_PORT 5060

// SIP Client States
typedef enum {
    SIP_STATE_IDLE,
    SIP_STATE_REGISTERING,
    SIP_STATE_REGISTERED,
    SIP_STATE_CALLING,
    SIP_STATE_IN_CALL,
    SIP_STATE_ERROR
} sip_state_t;

// SIP Authentication
typedef struct {
    char realm[64];
    char nonce[64];
    char algorithm[16];
    char qop[16];
    bool auth_required;
} sip_auth_t;

// SIP Call Information
typedef struct {
    char call_id[64];
    char from_tag[32];
    char to_tag[32];
    uint32_t cseq;
    bool active;
} sip_call_info_t;

// Main SIP Client Structure
typedef struct {
    // Network Configuration
    char server_ip[SIP_MAX_IP_LEN];
    uint16_t server_port;
    char local_ip[SIP_MAX_IP_LEN];
    uint16_t local_port;
    
    // SIP Credentials
    char username[SIP_MAX_USERNAME_LEN];
    char password[SIP_MAX_PASSWORD_LEN];
    char display_name[SIP_MAX_USERNAME_LEN];
    char domain[SIP_MAX_IP_LEN];
    
    // Network
    int socket_fd;
    struct sockaddr_in server_addr;
    
    // State Management
    sip_state_t state;
    sip_auth_t auth;
    sip_call_info_t call_info;
    
    // Counters
    uint32_t call_id_counter;
    uint32_t cseq_counter;
    uint32_t register_expires;
    
    // Callbacks
    void (*on_registration_success)(void);
    void (*on_registration_failed)(void);
    void (*on_incoming_call)(const char* caller_id);
    void (*on_call_ended)(void);
    void (*on_dtmf_received)(char dtmf_code);
    
} sip_client_t;

// Core Functions
esp_err_t sip_client_init(sip_client_t* client);
esp_err_t sip_client_start(sip_client_t* client);
esp_err_t sip_client_stop(sip_client_t* client);
esp_err_t sip_client_register(sip_client_t* client);
esp_err_t sip_client_answer_call(sip_client_t* client);
esp_err_t sip_client_hangup_call(sip_client_t* client);

// Message Processing
esp_err_t sip_client_process_message(sip_client_t* client, char* buffer, int len, struct sockaddr_in* from);
esp_err_t sip_client_send_response(sip_client_t* client, int status_code, const char* reason, const char* call_id);

// Utility Functions
void sip_client_set_callbacks(sip_client_t* client,
                             void (*on_reg_success)(void),
                             void (*on_reg_failed)(void),
                             void (*on_incoming_call)(const char*),
                             void (*on_call_ended)(void),
                             void (*on_dtmf)(char));

const char* sip_state_to_string(sip_state_t state);

#endif // SIP_CLIENT_H
