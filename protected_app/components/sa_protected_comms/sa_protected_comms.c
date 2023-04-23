#include "sa_protected_comms.h"

static SemaphoreHandle_t mutexes[SA_MAX_PROTECTED_MUTEXES];

void sa_protected_comms_init()
{
    for (int i = 0; i < SA_MAX_PROTECTED_MUTEXES; i++)
    {
        mutexes[i] = NULL;
    }
}

int sa_protected_mutex_create()
{
    for (int i = 0; i < SA_MAX_PROTECTED_MUTEXES; i++)
    {
        if (mutexes[i] == NULL)
        {
            mutexes[i] = xSemaphoreCreateMutex();
            return i;
        }
    }
    return -1;
}

void sa_protected_mutex_destroy(int mutex_handle)
{
    if (mutexes[mutex_handle] != NULL)
    {
        vSemaphoreDelete(mutexes[mutex_handle]);
        mutexes[mutex_handle] = NULL;
    }
}

bool sa_protected_mutex_lock(int mutex_handle)
{
    if (mutexes[mutex_handle] == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(mutexes[mutex_handle], portMAX_DELAY) == pdTRUE)
    {
        return true;
    }

    return false;
}

void sa_protected_mutex_unlock(int mutex_handle)
{
    if (mutexes[mutex_handle] != NULL)
    {
        xSemaphoreGive(mutexes[mutex_handle]);
    }
} 

void sa_protected_send(int sock, int sock_mutex, uint8_t *data, size_t data_len)
{
    if (sa_protected_mutex_lock(sock_mutex) == true)
    {
        send(sock, data, data_len, 0);
        sa_protected_mutex_unlock(sock_mutex);
    }
}

void sa_protected_recv(int sock, int sock_mutex, uint8_t *rx_buf, size_t len)
{
    if (sa_protected_mutex_lock(sock_mutex) == true)
    {
        recv(sock, rx_buf, len, MSG_WAITALL);
        sa_protected_mutex_unlock(sock_mutex);
    }
}

void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, int *mutexes, size_t num_sockets)
{
    if (sockets == NULL || mutexes == NULL || num_sockets == 0)
    {
        return;
    }

    for (int i = 0; i < num_sockets; i++)
    {
        sa_protected_send(sockets[i], mutexes[i], data, data_len);
    }
}