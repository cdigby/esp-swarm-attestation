#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "sa_shared_defs.h"

// void wifi_init_config_default(wifi_init_config_t *cfg);

void simple_prover(uint8_t msg[SIMPLE_MSG_LEN], uint8_t h[SIMPLE_HMAC_LEN], int response_sock, int response_sock_mutex);

uint32_t sa_network_get_gateway_ip();

int sa_protected_mutex_create();
void sa_protected_mutex_destroy(int mutex_handle);
bool sa_protected_mutex_lock(int mutex_handle);
void sa_protected_mutex_unlock(int mutex_handle);
