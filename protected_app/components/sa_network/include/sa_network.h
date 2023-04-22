#pragma once

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "esp_event_base.h"
#include "esp_event.h"

#include "sa_build_config.h"

static const char *TAG_NETWORK = "NETWORK LOG";

void sa_network_init();
uint32_t sa_network_get_gateway_ip();