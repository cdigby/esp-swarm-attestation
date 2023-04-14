#pragma once

#include <stdint.h>

#include "esp_wifi.h"

// typedef struct
// {
//     c;  //counter
//     vs; //valids state
//     nonce;  //nonce
// } simple_msg_t;

void wifi_init_config_default(wifi_init_config_t *cfg);

// void execute_simple(simple_msg_t msg, h, int sender);
