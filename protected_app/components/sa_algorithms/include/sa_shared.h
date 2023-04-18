#pragma once

#include <stdint.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "Hacl_HMAC.h"

void init_fake_app_memory();
void compute_software_state(uint8_t *key, size_t key_len, uint8_t out[32]);