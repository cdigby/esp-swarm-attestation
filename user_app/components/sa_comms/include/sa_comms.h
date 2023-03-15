#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "errno.h"

#include "esp_log.h"
#include "esp_err.h"

#include "sa_build_config.h"
#include "sa_network.h"

#define CMD_NODE_NAME               0x01    // Transmit the node's name
#define CMD_HEARTBEAT_REQUEST       0x02
#define CMD_HEARTBEAT_RESPONSE      0x03

#define CMD_PRINT_MESSAGE           0x04

#define COMMS_TCP_PORT      3333
#define COMMS_BUFFER_SIZE   256     // Size in bytes of tx and rx buffers for TCP client and server
#define COMMS_QUEUE_LENGTH  32      // Number of commands that each connection can have queued for transmission

#define COMMS_HEARTBEAT_TIMEOUT_MS   10000

#define TCP_SERVER_MAX_CONNS      10

#define TCP_SERVER_KEEPALIVE_IDLE         5
#define TCP_SERVER_KEEPALIVE_INTERVAL     5
#define TCP_SERVER_KEEPALIVE_COUNT        10

typedef struct
{
    uint8_t cmd_code;   // Command code
    uint8_t data_len;   // If > 0, data points to extra data that must be freed after processing
    uint8_t *data;
} comms_cmd_t;

typedef struct
{
    bool open;
    int sock;
    bool heartbeat;
    char name[64];
    QueueHandle_t cmd_queue;
} tcp_conn_t;

typedef struct
{
    int listen_sock;

    tcp_conn_t conns[TCP_SERVER_MAX_CONNS];
    int num_conns;
    int64_t heartbeat_timer;

    uint8_t tx_buf[COMMS_BUFFER_SIZE];
    uint8_t rx_buf[COMMS_BUFFER_SIZE];
} tcp_server_t;

typedef struct
{
    char node_name[64];
    tcp_conn_t server;
    int64_t heartbeat_timer;

    uint8_t tx_buf[COMMS_BUFFER_SIZE];
    uint8_t rx_buf[COMMS_BUFFER_SIZE];
} tcp_client_t;

static const char *TAG_COMMS = "COMMS LOG";

bool sa_comms_init();
void sa_comms_broadcast(comms_cmd_t *cmd);