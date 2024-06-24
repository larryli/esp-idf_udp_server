#pragma once

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(UDP_EVENT);

enum {
    UDP_EVENT_RECV,
    UDP_EVENT_SEND,
    UDP_EVENT_BROADCAST,
};

typedef struct {
    void *remote;
    void *payload;
    size_t len;
} udp_msg_t;

typedef struct {
    int port;
    void *payload;
    size_t len;
} udp_broadcast_t;

esp_err_t udp_server_init(int port);
esp_err_t udp_server_deinit(void);

bool udp_remote_eq(const void *remote1, const void *remote2);
void *udp_remote_clone(const void *remote);
void udp_remote_free(void *remote);
char *udp_remote_ipstr(const void *remote);
int udp_remote_port(const void *remote);

#ifdef __cplusplus
}
#endif
