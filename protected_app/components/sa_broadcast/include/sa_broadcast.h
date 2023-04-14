#pragma once

#include <stdint.h>

#include "lwip/err.h"
#include "lwip/sockets.h"

// Broadcast a message from within the protected app
// data contains the message data
// data_len is the number of bytes to transmit
// sockets is an array containing the socket handles to broadcast to
// num_sockets is the number of sockets contained in the sockets array
//
// To minimize complexity of the code running in protected space, no checks are done to ensure
// that the sockets are still alive etc. This should be handled otherwise by the userspace network stack
void sa_protected_broadcast(uint8_t *data, size_t data_len, int *sockets, size_t num_sockets);

void sa_protected_unicast(uint8_t *data, size_t data_len, int *socket);