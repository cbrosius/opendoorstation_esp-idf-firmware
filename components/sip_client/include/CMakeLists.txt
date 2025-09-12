idf_component_register(
    SRCS "sip_client.c"
    INCLUDE_DIRS "include"
    REQUIRES "esp_netif" "lwip" "mbedtls"
)
