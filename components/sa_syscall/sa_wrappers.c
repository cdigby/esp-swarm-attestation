#include <stdint.h>
#include <stdbool.h>

#include "sa_syscall.h"

#include "syscall_macros.h"
#include "syscall_def.h"

#include "esp_err.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"

// Compiler will give this warning about all the syscall wrappers,
// esp-privilege-separation/components/user/syscall_wrapper/syscall_wrappers.c ignores them too.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wint-conversion"
#endif

// esp_netif_t *usr_esp_netif_create_default_wifi_ap()
// {
//     return EXECUTE_SYSCALL(__NR_esp_netif_create_default_wifi_ap);
// }

// esp_err_t usr_esp_netif_get_ip_info(esp_netif_t *esp_netif, esp_netif_ip_info_t *ip_info)
// {
//     return EXECUTE_SYSCALL(esp_netif, ip_info, __NR_esp_netif_get_ip_info);
// }

// esp_err_t usr_esp_netif_set_ip_info(esp_netif_t *esp_netif, const esp_netif_ip_info_t *ip_info)
// {
//     return EXECUTE_SYSCALL(esp_netif, ip_info, __NR_esp_netif_set_ip_info);
// }

// esp_err_t usr_esp_netif_dhcps_stop(esp_netif_t *esp_netif)
// {
//     return EXECUTE_SYSCALL(esp_netif, __NR_esp_netif_dhcps_stop);
// }

// esp_err_t usr_esp_netif_dhcps_start(esp_netif_t *esp_netif)
// {
//     return EXECUTE_SYSCALL(esp_netif, __NR_esp_netif_dhcps_start);
// }

// void usr_wifi_init_config_default(wifi_init_config_t *cfg)
// {
//     EXECUTE_SYSCALL(__NR_wifi_init_config_default);
// }

void usr_simple_prover(uint8_t *msg, size_t msg_len, int response_sock)
{
    EXECUTE_SYSCALL(msg, msg_len, response_sock, __NR_simple_prover);
}

void usr_simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, size_t num_sockets)
{
    EXECUTE_SYSCALL(attest_req, attest_req_len, sockets, num_sockets, __NR_simple_plus_prover_attest);
}

void usr_simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int *sockets, size_t num_sockets)
{
    EXECUTE_SYSCALL(collect_req, collect_req_len, sender_sock, sockets, num_sockets, __NR_simple_plus_prover_collect);
}

uint32_t usr_sa_network_get_gateway_ip()
{
    return EXECUTE_SYSCALL(__NR_sa_network_get_gateway_ip);
}

int usr_sa_protected_mutex_create()
{
    return EXECUTE_SYSCALL(__NR_sa_protected_mutex_create);
}

void usr_sa_protected_mutex_destroy(int mutex_handle)
{
    EXECUTE_SYSCALL(mutex_handle, __NR_sa_protected_mutex_destroy);
}

bool usr_sa_protected_mutex_lock(int mutex_handle)
{
    return EXECUTE_SYSCALL(mutex_handle, __NR_sa_protected_mutex_lock);
}

void usr_sa_protected_mutex_unlock(int mutex_handle)
{
    EXECUTE_SYSCALL(mutex_handle, __NR_sa_protected_mutex_unlock);
}
