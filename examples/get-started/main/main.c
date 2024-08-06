/* UDP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "udp_server.h"
#include <string.h>

#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "get_start";

static void on_recv(void *handler_arg, esp_event_base_t base, int32_t id,
                    void *event_data)
{
    if (event_data == NULL) {
        return;
    }
    udp_msg_t *recv = (udp_msg_t *)event_data;
    ESP_LOGI(TAG, "Received %d bytes from %s:%d", recv->len,
             udp_remote_ipstr(recv->remote), udp_remote_port(recv->remote));
    ESP_LOG_BUFFER_HEXDUMP(TAG, recv->payload, recv->len, ESP_LOG_INFO);

#if 0
    if (esp_event_post(UDP_EVENT, UDP_EVENT_SEND, recv, sizeof(udp_msg_t),
                       20 / portTICK_PERIOD_MS) != ESP_OK) {
        free(recv->remote);
        free(recv->payload);
    }
#else
    udp_msg_t send = {
        .len = recv->len,
    };
    send.remote = udp_remote_clone(recv->remote);
    if (send.remote != NULL) {
        send.payload = malloc(send.len);
        if (send.payload != NULL) {
            memcpy(send.payload, recv->payload, send.len);
            if (esp_event_post(UDP_EVENT, UDP_EVENT_SEND, &send,
                               sizeof(udp_msg_t),
                               20 / portTICK_PERIOD_MS) != ESP_OK) {
                free(send.remote);
                free(send.payload);
            }
        } else {
            free(send.remote);
        }
    }

    free(recv->remote);
    free(recv->payload);
#endif
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in
     * menuconfig. Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    ESP_ERROR_CHECK(udp_server_init(PORT));
    ESP_ERROR_CHECK(
        esp_event_handler_register(UDP_EVENT, UDP_EVENT_RECV, on_recv, NULL));
}
