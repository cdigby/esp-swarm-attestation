#pragma once

#include <stdint.h>

#include "esp_wifi.h"

void wifi_init_config_default(wifi_init_config_t *cfg);

void simple_prover(uint8_t msg[68], uint8_t h[32], int response_sock);
