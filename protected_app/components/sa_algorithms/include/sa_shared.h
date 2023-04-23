#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "sa_build_config.h"

#include "Hacl_HMAC.h"

#define SIMPLE_KEY_SIZE     32
#define SIMPLE_HMAC_LEN     32

void init_fake_app_memory();
void compute_software_state(uint8_t *key, size_t key_len, uint8_t out[SIMPLE_HMAC_LEN]);