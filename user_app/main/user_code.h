#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "sa_syscall.h"

static const char *TAG_USER = "USERSPACE LOG";