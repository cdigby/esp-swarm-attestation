#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "sa_network.h"

static const char *TAG_SYSCALL = "SYSCALL LOG";

int sys_sa_network_get_gateway_addr(uint32_t *result)
{
    return sa_network_get_gateway_addr(result);
}

