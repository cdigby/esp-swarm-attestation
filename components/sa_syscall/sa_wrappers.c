#include "sa_syscall.h"

#include "syscall_macros.h"
#include "syscall_def.h"

int usr_sa_network_get_gateway_ip(uint32_t *result)
{
    return EXECUTE_SYSCALL(result, __NR_sa_network_get_gateway_ip);
}
