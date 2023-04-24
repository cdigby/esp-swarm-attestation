#include "sa_shared.h"

static uint8_t fake_app_memory[NODE_FAKE_MEMORY_SIZE];

void init_fake_app_memory()
{
    memset(fake_app_memory, NODE_FAKE_MEMORY_CONTENTS, NODE_FAKE_MEMORY_SIZE);
}

void compute_software_state(uint8_t *key, size_t key_len, uint8_t out[SIMPLE_HMAC_LEN])
{
    Hacl_HMAC_compute_sha2_256(out, key, key_len, fake_app_memory, NODE_FAKE_MEMORY_SIZE);
}