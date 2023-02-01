// This file implements the kernel-space functionality of our syscalls

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG_SYSCALL = "SYSCALL LOG";

int sys_test_syscall(void *args)
{
    ESP_LOGI(TAG_SYSCALL, "THIS IS A SYSCALL!\n");
    return 0;
}

