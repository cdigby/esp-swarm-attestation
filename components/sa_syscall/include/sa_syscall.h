#pragma once

#include <stdint.h>

#include "esp_wifi.h"

#include "sa_shared_defs.h"

void wifi_init_config_default(wifi_init_config_t *cfg);

void simple_prover(uint8_t msg[SIMPLE_MSG_LEN], uint8_t h[SIMPLE_HMAC_LEN], int response_sock);
