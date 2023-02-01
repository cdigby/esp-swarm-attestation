// This file implements the userspace wrappers for our syscalls

#include "sa_syscall.h"

#include "syscall_macros.h"
#include "syscall_def.h"

int usr_test_syscall(void *args)
{
    return EXECUTE_SYSCALL(args, __NR_test_syscall);
}
