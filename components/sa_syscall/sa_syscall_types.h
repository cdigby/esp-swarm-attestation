#pragma once

typedef struct
{
    uint8_t *collect_req;
    size_t collect_req_len;
    int sender_sock;
    int sender_mutex;
    int *sockets;
    int *mutexes;
    size_t num_sockets;
} sa_simple_plus_prover_collect_args_t;