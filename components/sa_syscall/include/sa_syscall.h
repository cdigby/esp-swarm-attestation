#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// void wifi_init_config_default(wifi_init_config_t *cfg);

void simple_prover(uint8_t *msg, size_t msg_len, int response_sock, int response_sock_mutex);
void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, int *mutexes, size_t num_sockets);
void simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int sender_mutex, int *sockets, int *mutexes, size_t num_sockets);

uint32_t sa_network_get_gateway_ip();

int sa_protected_mutex_create();
void sa_protected_mutex_destroy(int mutex_handle);
bool sa_protected_mutex_lock(int mutex_handle);
void sa_protected_mutex_unlock(int mutex_handle);
