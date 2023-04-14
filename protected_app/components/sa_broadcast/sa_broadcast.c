#include "sa_broadcast.h"

void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, size_t num_sockets)
{
    for (int i = 0; i < num_sockets; i++)
    {
        send(sockets[i], data, data_len, 0);
    }
}

// void sa_protected_unicast