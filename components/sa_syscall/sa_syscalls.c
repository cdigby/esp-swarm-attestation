#include <stdio.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"

esp_netif_t *sys_esp_netif_create_default_wifi_ap()
{
    return esp_netif_create_default_wifi_ap();
}

esp_err_t sys_esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info)
{
    return esp_netif_get_ip_info(esp_netif, ip_info);
}

esp_err_t sys_esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info)
{
    return esp_netif_set_ip_info(esp_netif, ip_info);
}

esp_err_t sys_esp_netif_dhcps_stop(esp_netif_t *esp_netif)
{
    return esp_netif_dhcps_stop(esp_netif);
}

esp_err_t sys_esp_netif_dhcps_start(esp_netif_t *esp_netif)
{
    return esp_netif_dhcps_start(esp_netif);
}

void sys_wifi_init_config_default(wifi_init_config_t *cfg)
{
    // Implementing as syscalls the functions this macro calls would have been a nightmare
    // For some reason, we can't return a wifi_init_config_t from a syscall, so memcpy into cfg as an output
    wifi_init_config_t new = WIFI_INIT_CONFIG_DEFAULT();
    memcpy(cfg, &new, sizeof(wifi_init_config_t));
}