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

#include "sa_build_config.h"
#include "sa_protected_comms.h"
#include "sa_network.h"
#include "sa_shared.h"

static const char *TAG_PROTECTED = "PROTECTED APP LOG";