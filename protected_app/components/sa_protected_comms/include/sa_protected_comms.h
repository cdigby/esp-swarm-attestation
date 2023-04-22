#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

// Functions for sending an receiving data from within the protected app
// Management of sockets including connection, disconnection and aliveness checkes should
// be handled by userspace network stack
// If sock_mutex is not NULL, then the socket will only be operated on when the mutex is acquired
void sa_protected_send(int sock, uint8_t *data, size_t data_len, SemaphoreHandle_t sock_mutex);
void sa_protected_recv(int sock, uint8_t *rx_buf, size_t len, SemaphoreHandle_t sock_mutex);
void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, SemaphoreHandle_t *mutexes, size_t num_sockets);
