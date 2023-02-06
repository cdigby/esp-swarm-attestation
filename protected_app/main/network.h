#pragma once

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif_types.h"
#include "esp_wifi_types.h"
#include "esp_event_base.h"
#include "esp_event.h"

#include "build_config.h"

static const char *TAG_NETWORK = "NETWORK LOG";

void initialise_wifi();