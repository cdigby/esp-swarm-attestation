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

#include "build_config.h"
#include "network.h"

static const char *TAG_KERNEL = "KERNEL LOG";