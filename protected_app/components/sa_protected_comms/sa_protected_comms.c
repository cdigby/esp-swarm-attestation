#include "sa_protected_comms.h"

void sa_protected_send(int sock, uint8_t *data, size_t data_len, SemaphoreHandle_t sock_mutex)
{
    if (sock_mutex != NULL)
    {
        if (xSemaphoreTake(sock_mutex, portMAX_DELAY) == pdTRUE)
        {
            send(sock, data, data_len, 0);
            xSemaphoreGive(sock_mutex);
        }
    }
    else
    {
        send(sock, data, data_len, 0);
    }    
}

void sa_protected_recv(int sock, uint8_t *rx_buf, size_t len, SemaphoreHandle_t sock_mutex)
{
    if (sock_mutex != NULL)
    {
        if (xSemaphoreTake(sock_mutex, portMAX_DELAY) == pdTRUE)
        {
            recv(sock, rx_buf, len, MSG_WAITALL);
            xSemaphoreGive(sock_mutex);
        }
    }
    else
    {
        recv(sock, rx_buf, len, MSG_WAITALL);
    }
}

void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, SemaphoreHandle_t *mutexes, size_t num_sockets)
{
    for (int i = 0; i < num_sockets; i++)
    {
        sa_protected_send(sockets[i], data, data_len, mutexes[i]);
    }
}