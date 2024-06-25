# UDP Server component for esp-idf

Send & Receive messages on default event loop.

## Installation

## Getting Started

    idf.py add-dependency "larryli/udp_server"

### Init & Deinit

```c
#include "htpad_server.h"
#define PORT 10000
    udp_server_init(PORT);
    udp_server_deinit();
```

### Receive message

```c
static void on_recv(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (event_data == NULL) {
        return;
    }
    udp_msg_t *msg = (udp_msg_t *)event_data;
    ESP_LOGI(TAG, "Received %d bytes from %s:%d", msg->len, udp_remote_ipstr(msg->remote), udp_remote_port(msg->remote));
    ESP_LOG_BUFFER_HEXDUMP(TAG, msg->payload, msg->len, ESP_LOG_INFO);
    free(msg->remote);
    free(msg->payload);
}

    esp_event_handler_register(UDP_EVENT, UDP_EVENT_RECV, on_recv, NULL);
```

### Save remote ip & port

```c
    void *remote = udp_remote_clone(msg->remote);
    if (udp_remote_eq(msg->remote, remote)) {
        // is saved remote
    }
    free(remote);
```

### Send message

```c
    char *str = "This is a message.";
    udp_msg_t msg = {
        .len = strlen(str),
    };
    msg.remote = udp_remote_clone(remote);
    if (msg.remote != NULL) {
        msg.payload = malloc(msg.len);
        if (msg.payload != NULL) {
            memcpy(msg.payload, str, msg.len);
            if (esp_event_post(UDP_EVENT, UDP_EVENT_SEND, &msg, sizeof(udp_msg_t), 20 / portTICK_PERIOD_MS) != ESP_OK) {
                free(msg.remote);
                free(msg.payload);
            }
        } else {
            free(msg.remote);
        }
    }
```
