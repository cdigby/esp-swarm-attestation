#include "sa_protected_comms.h"

void sa_protected_send(int sock, uint8_t *data, size_t data_len)
{
    send(sock, data, data_len, 0);
}

void sa_protected_recv(int sock, uint8_t *rx_buf, size_t len)
{
    recv(sock, rx_buf, len, MSG_WAITALL);
}

void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, size_t num_sockets)
{
    for (int i = 0; i < num_sockets; i++)
    {
        sa_protected_send(sockets[i], data, data_len);
    }
}