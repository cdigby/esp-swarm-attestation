#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sockets.h"

#define SA_MAX_PROTECTED_MUTEXES 64

// Functions for sending an receiving data from within the protected app
// Management of sockets including connection, disconnection and aliveness checks should
// be handled by userspace network stack

void sa_protected_comms_init();
int sa_protected_mutex_create();
void sa_protected_mutex_destroy(int mutex_handle);
bool sa_protected_mutex_lock(int mutex_handle);
void sa_protected_mutex_unlock(int mutex_handle);

void sa_protected_send(int sock, int sock_mutex, uint8_t *data, size_t data_len);
int sa_protected_recv(int sock, int sock_mutex, uint8_t *rx_buf, size_t len);
void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, int *mutexes, size_t num_sockets);
