#include "sa_shared.h"

#define FAKE_APP_MEMORY_SIZE 1024
static uint8_t fake_app_memory[FAKE_APP_MEMORY_SIZE];

void init_fake_app_memory()
{
    memset(fake_app_memory, 0, FAKE_APP_MEMORY_SIZE);
}

void compute_software_state(uint8_t *key, size_t key_len, uint8_t out[32])
{
    Hacl_HMAC_compute_sha2_256(out, key, key_len, fake_app_memory, FAKE_APP_MEMORY_SIZE);
}