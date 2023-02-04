#pragma once

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_attr.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_priv_access.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif_types.h"
#include "esp_wifi_types.h"
#include "esp_event_base.h"
#include "esp_event.h"

static const char *TAG_KERNEL = "KERNEL LOG";