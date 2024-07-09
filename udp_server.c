#include "udp_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "lwip/def.h"
#include "lwip/err.h"
#include "lwip/udp.h"

static const char *TAG = "udp_server";

ESP_EVENT_DEFINE_BASE(UDP_EVENT);

typedef struct {
    ip_addr_t addr;
    uint16_t port;
} remote_t;

static struct udp_pcb *pcb = NULL;

static void udp_server_on_send(void *handler_arg, esp_event_base_t base,
                               int32_t id, void *event_data)
{
    if (pcb == NULL || event_data == NULL) {
        return;
    }

    udp_msg_t *msg = (udp_msg_t *)event_data;
    remote_t *target = (remote_t *)msg->remote;
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
    memcpy(send_pb->payload, msg->payload, msg->len);
    ESP_LOGD(TAG, "Send %d bytes to %s:%d", send_pb->len,
             ipaddr_ntoa(&target->addr), target->port);
    ESP_LOG_BUFFER_HEXDUMP(TAG, send_pb->payload, send_pb->len, ESP_LOG_DEBUG);
    udp_sendto(pcb, send_pb, &target->addr, target->port);
    pbuf_free(send_pb);

    free(msg->remote);
    free(msg->payload);
}

static void udp_server_on_broadcast(void *handler_arg, esp_event_base_t base,
                                    int32_t id, void *event_data)
{
    if (pcb == NULL || event_data == NULL) {
        return;
    }

    udp_broadcast_t *msg = (udp_broadcast_t *)event_data;
    struct pbuf *send_pb = pbuf_alloc(PBUF_TRANSPORT, msg->len, PBUF_RAM);
    memcpy(send_pb->payload, msg->payload, msg->len);
    ESP_LOGD(TAG, "Broadcast %d bytes to port %d", send_pb->len, msg->port);
    ESP_LOG_BUFFER_HEXDUMP(TAG, send_pb->payload, send_pb->len, ESP_LOG_DEBUG);
    udp_sendto(pcb, send_pb, IP_ADDR_BROADCAST, msg->port);
    pbuf_free(send_pb);

    free(msg->payload);
}

static void udp_recv_proc(void *arg, struct udp_pcb *pcb, struct pbuf *pb,
                          const ip_addr_t *addr, uint16_t port)
{
    while (pb != NULL) {
        struct pbuf *this_pb = pb;
        pb = pb->next;
        this_pb->next = NULL;
        ESP_LOGD(TAG, "Received %d bytes from %s:%d", this_pb->len,
                 ipaddr_ntoa(addr), port);
        ESP_LOG_BUFFER_HEXDUMP(TAG, this_pb->payload, this_pb->len,
                               ESP_LOG_DEBUG);
        udp_msg_t msg = {
            .len = this_pb->len,
        };
        msg.remote = malloc(sizeof(remote_t));
        if (msg.remote == NULL) {
            goto end;
        }
        msg.payload = malloc(this_pb->len);
        if (msg.payload == NULL) {
            free(msg.remote);
            goto end;
        }
        remote_t *remote = (remote_t *)msg.remote;
        ip_addr_copy(remote->addr, *addr);
        remote->port = port;
        memcpy(msg.payload, this_pb->payload, this_pb->len);
        if (esp_event_post(UDP_EVENT, UDP_EVENT_RECV, &msg, sizeof(udp_msg_t),
                           20 / portTICK_PERIOD_MS) != ESP_OK) {
            free(msg.remote);
            free(msg.payload);
        }

    end:
        pbuf_free(this_pb);
    }
}

esp_err_t udp_server_init(int port)
{
    esp_err_t err = ESP_OK;
    if (pcb != NULL) {
        ESP_LOGW(TAG, "Udp server is running");
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "Init udp server: %d", port);

    udp_init();
    pcb = udp_new();
    if (pcb == NULL) {
        ESP_LOGE(TAG, "Init udp server failed!");
        return ESP_ERR_NO_MEM;
    }
    err_t ret = udp_bind(pcb, IP_ADDR_ANY, port);
    if (ret != ERR_OK) {
        ESP_LOGE(TAG, "Bind udp server failed: %d", ret);
        err = ESP_ERR_INVALID_STATE;
        goto failed;
    }

    udp_recv(pcb, &udp_recv_proc, NULL);

    ret = esp_event_handler_register(UDP_EVENT, UDP_EVENT_SEND,
                                     udp_server_on_send, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register udp server send failed: %d", ret);
        goto failed;
    }
    ret = esp_event_handler_register(UDP_EVENT, UDP_EVENT_BROADCAST,
                                     udp_server_on_broadcast, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Register udp server broadcast failed: %d", ret);
        goto failed;
    }

    return ESP_OK;

failed:
    udp_server_deinit();
    return err;
}

esp_err_t udp_server_deinit(void)
{
    esp_event_handler_unregister(UDP_EVENT, UDP_EVENT_SEND, udp_server_on_send);
    esp_event_handler_unregister(UDP_EVENT, UDP_EVENT_BROADCAST,
                                 udp_server_on_broadcast);
    if (pcb != NULL) {
        udp_remove(pcb);
        pcb = NULL;
    }
    return ESP_OK;
}

bool udp_remote_eq(const void *remote1, const void *remote2)
{
    if (remote1 == NULL || remote2 == NULL) {
        return false;
    }
    remote_t *target1 = (remote_t *)remote1;
    remote_t *target2 = (remote_t *)remote2;
    if (target1->port != target2->port) {
        return false;
    }
    return ip_addr_cmp(&target1->addr, &target1->addr);
}

void *udp_remote_clone(const void *remote)
{
    if (remote == NULL) {
        return NULL;
    }
    remote_t *target = (remote_t *)remote;
    remote_t *clone = malloc(sizeof(remote_t));
    if (clone == NULL) {
        return NULL;
    }
    ip_addr_copy(clone->addr, target->addr);
    clone->port = target->port;
    return clone;
}

char *udp_remote_ipstr(const void *remote)
{
    if (remote == NULL) {
        return NULL;
    }
    remote_t *target = (remote_t *)remote;
    return ipaddr_ntoa(&target->addr);
}

int udp_remote_port(const void *remote)
{
    if (remote == NULL) {
        return -1;
    }
    remote_t *target = (remote_t *)remote;
    return target->port;
}
