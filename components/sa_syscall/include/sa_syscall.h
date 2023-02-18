#pragma once

#include <stdint.h>

// If the node is connected to its parent, set result to the parent's gateway address
// Returns 0 if successful, otherwise -1
int usr_sa_network_get_gateway_addr(uint32_t *result);