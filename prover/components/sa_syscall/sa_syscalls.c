#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "soc_defs.h"

// #include "esp_map.h"
// #include "esp_netif.h"
// #include "esp_wifi.h"


// // Adapted from sys_esp_netif_create_default_wifi_sta() in esp_privilege_separation/components/protected/src/esp_syscalls.c
// esp_netif_t *sys_esp_netif_create_default_wifi_ap()
// {
//     esp_netif_t *handle = esp_netif_create_default_wifi_ap();
//     int wrapper_index = esp_map_add(handle, ESP_MAP_ESP_NETIF_ID);
//     if (!wrapper_index) {
//         esp_netif_destroy(handle);
//         return NULL;
//     }
//     return (esp_netif_t *)wrapper_index;
// }

// // esp_netif is actually an esp_map index (see esp_privilege_separation/components/protected/src/esp_map.c), not a real pointer
// // We need to get the actual pointer to esp_netif from esp_map before we can operate on it 
// esp_err_t sys_esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info)
// {
//     esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
//     if (!mh)
//     {
//         return ESP_FAIL;
//     }

//     return esp_netif_get_ip_info((esp_netif_t *)mh->handle, ip_info);
// }

// esp_err_t sys_esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info)
// {
//     esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
//     if (!mh)
//     {
//         return ESP_FAIL;
//     }

//     return esp_netif_set_ip_info((esp_netif_t *)mh->handle, ip_info);
// }

// esp_err_t sys_esp_netif_dhcps_stop(esp_netif_t *esp_netif)
// {
//     esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
//     if (!mh)
//     {
//         return ESP_FAIL;
//     }

//     return esp_netif_dhcps_stop((esp_netif_t *)mh->handle);
// }

// esp_err_t sys_esp_netif_dhcps_start(esp_netif_t *esp_netif)
// {
//     esp_map_handle_t *mh = esp_map_get_handle((int)esp_netif);
//     if (!mh)
//     {
//         return ESP_FAIL;
//     }

//     return esp_netif_dhcps_start((esp_netif_t *)mh->handle);
// }

// // Implementing as syscalls the functions this macro calls would have been a nightmare
// // We can't return a wifi_init_config_t from a syscall as it cannot be casted to an int, so memcpy into cfg as an output
// void sys_wifi_init_config_default(wifi_init_config_t *cfg)
// {
//     wifi_init_config_t new = WIFI_INIT_CONFIG_DEFAULT();
//     memcpy(cfg, &new, sizeof(wifi_init_config_t));
// }

// We will link against the sa_algorithms component library, but can't easily include the headers,
// so forward declare the functions we need as extern
extern void simple_prover(uint8_t *msg, size_t msg_len, int response_sock);
void sys_simple_prover(uint8_t *msg, size_t msg_len, int response_sock)
{
    // Verify that the bounds of msg are valid userspace DRAM addresses
    if (!is_valid_udram_addr((void *)msg) || !is_valid_udram_addr((void *)msg + msg_len))
    {
        return;
    }

    simple_prover(msg, msg_len, response_sock);
}

extern void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, size_t num_sockets);
void sys_simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, size_t num_sockets)
{
    // Verify that the bounds of attest_req and sockets are valid userspace DRAM addresses
    if (!is_valid_udram_addr((void *)attest_req) || !is_valid_udram_addr((void *)attest_req + attest_req_len))
    {
        return;
    }
    if (!is_valid_udram_addr((void *)sockets) || !is_valid_udram_addr((void *)sockets + num_sockets))
    {
        return;
    }

    simple_plus_prover_attest(attest_req, attest_req_len, sockets, num_sockets);
}

extern void simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int *sockets, size_t num_sockets);
void sys_simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int *sockets, size_t num_sockets)
{
    // Verify that the bounds of collect_req and sockets are valid userspace DRAM addresses
    if (!is_valid_udram_addr((void *)collect_req) || !is_valid_udram_addr((void *)collect_req + collect_req_len))
    {
        return;
    }
    if (!is_valid_udram_addr((void *)sockets) || !is_valid_udram_addr((void *)sockets + num_sockets))
    {
        return;
    }

    simple_plus_prover_collect(collect_req, collect_req_len, sender_sock, sockets, num_sockets);
}

// Return the gateway ip of the parent node
// If not connected, 0 will be returned
extern uint32_t sa_network_get_gateway_ip();
uint32_t sys_sa_network_get_gateway_ip()
{
    return sa_network_get_gateway_ip();
}

extern int sa_protected_mutex_create();
int sys_sa_protected_mutex_create()
{
    return sa_protected_mutex_create();
}

extern void sa_protected_mutex_destroy(int mutex_handle);
void sys_sa_protected_mutex_destroy(int mutex_handle)
{
    sa_protected_mutex_destroy(mutex_handle);
}

extern bool sa_protected_mutex_lock(int mutex_handle);
bool sys_sa_protected_mutex_lock(int mutex_handle)
{
    return sa_protected_mutex_lock(mutex_handle);
}

extern void sa_protected_mutex_unlock(int mutex_handle);
void sys_sa_protected_mutex_unlock(int mutex_handle)
{
    sa_protected_mutex_unlock(mutex_handle);
}