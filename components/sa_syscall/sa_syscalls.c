#include <stdio.h>
#include <string.h>

#include "esp_map.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"

// Adapted from sys_esp_netif_create_default_wifi_sta() in esp_privilege_separation/components/protected/src/esp_syscalls.c
esp_netif_t *sys_esp_netif_create_default_wifi_ap()
{
    esp_netif_t *handle = esp_netif_create_default_wifi_ap();
    int wrapper_index = esp_map_add(handle, ESP_MAP_ESP_NETIF_ID);
    if (!wrapper_index) {
        esp_netif_destroy(handle);
        return NULL;
    }
    return (esp_netif_t *)wrapper_index;
}

// esp_netif is actually an esp_map index (see esp_privilege_separation/components/protected/src/esp_map.c), not a real pointer
// We need to get the actual pointer to esp_netif from esp_map before we can operate on it 
esp_err_t sys_esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info)
{
    esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
    if (!mh)
    {
        return ESP_FAIL;
    }

    return esp_netif_get_ip_info((esp_netif_t *)mh->handle, ip_info);
}

esp_err_t sys_esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info)
{
    esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
    if (!mh)
    {
        return ESP_FAIL;
    }

    return esp_netif_set_ip_info((esp_netif_t *)mh->handle, ip_info);
}

esp_err_t sys_esp_netif_dhcps_stop(esp_netif_t *esp_netif)
{
    esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
    if (!mh)
    {
        return ESP_FAIL;
    }

    return esp_netif_dhcps_stop((esp_netif_t *)mh->handle);
}

esp_err_t sys_esp_netif_dhcps_start(esp_netif_t *esp_netif)
{
    esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
    if (!mh)
    {
        return ESP_FAIL;
    }

    return esp_netif_dhcps_start((esp_netif_t *)mh->handle);
}

// Implementing as syscalls the functions this macro calls would have been a nightmare
// We can't return a wifi_init_config_t from a syscall as it cannot be casted to an int, so memcpy into cfg as an output
void sys_wifi_init_config_default(wifi_init_config_t *cfg)
{
    wifi_init_config_t new = WIFI_INIT_CONFIG_DEFAULT();
    memcpy(cfg, &new, sizeof(wifi_init_config_t));
}