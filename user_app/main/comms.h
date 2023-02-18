#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#include "errno.h"

#include "esp_log.h"
#include "esp_err.h"

#include "sa_build_config.h"
#include "sa_syscall.h"

#define CMD_NODE_NAME   0x01

#define COMMS_TCP_PORT      3333
#define COMMS_BUFFER_SIZE   256

#define TCP_HOST_MAX_CONNS      10

#define TCP_HOST_KEEPALIVE_IDLE         5
#define TCP_HOST_KEEPALIVE_INTERVAL     5
#define TCP_HOST_KEEPALIVE_COUNT        10

typedef struct
{
    bool open;
    int sock;
    char name[64];
} tcp_conn_t;

typedef struct
{
    int listen_sock;

    tcp_conn_t conns[TCP_HOST_MAX_CONNS];
    int num_conns;
    int64_t ping_timer;

    uint8_t tx_buf[COMMS_BUFFER_SIZE];
    uint8_t rx_buf[COMMS_BUFFER_SIZE];
} tcp_host_t;

typedef struct
{
    tcp_conn_t host;

    uint8_t tx_buf[COMMS_BUFFER_SIZE];
    uint8_t rx_buf[COMMS_BUFFER_SIZE];
} tcp_client_t;

static const char *TAG_TCP_HOST = "TCP HOST LOG";
static const char *TAG_TCP_CLIENT = "TCP CLIENT LOG";

bool tcp_host_start();
bool tcp_client_start();