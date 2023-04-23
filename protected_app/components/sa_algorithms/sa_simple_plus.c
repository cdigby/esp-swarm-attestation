#include "sa_simple_plus.h"

static const char *TAG_SIMPLE_PLUS = "SIMPLE+ LOG";

// STATIC DATA STORED IN PROTECTED MEMORY
// Marked as DRAM_ATTR to be certain that these are placed in RAM and not flash
static DRAM_ATTR uint8_t k_auth[SIMPLE_KEY_SIZE] =    // Authentication key
    {
        0x16, 0xD1, 0x44, 0xDE, 0x40, 0x56, 0xF1, 0x28, 0x8A, 0x5C, 0x37, 0xA9, 0x31, 0x40, 0x11, 0xDA,
        0xAB, 0xE8, 0x52, 0x5A, 0x09, 0x18, 0x53, 0xDB, 0xDB, 0xEE, 0x26, 0xF7, 0xD0, 0xD4, 0x44, 0x7E
    };

static DRAM_ATTR uint8_t k_attest[SIMPLE_KEY_SIZE] =    // Attestation key
    {
        0xD6, 0x0A, 0xC3, 0x21, 0x22, 0x39, 0x6A, 0xFF, 0xBA, 0x6A, 0x04, 0x02, 0x9D, 0x5A, 0xBE, 0xB7,
        0x10, 0x33, 0xD5, 0x67, 0x32, 0x36, 0xEE, 0x08, 0xA3, 0x35, 0x46, 0x96, 0x7D, 0xEB, 0x43, 0x65
    };

static DRAM_ATTR uint8_t k_col[SIMPLE_HMAC_LEN] =   // Collection key, updated by algorithm
    {                                               // Length must = SIMPLE_HMAC_LEN, as it is updated using HASH_SHA256(k_col)
        0x25, 0xC2, 0x6A, 0x89, 0x78, 0xCA, 0x86, 0x74, 0x8D, 0x55, 0x3D, 0xD4, 0x41, 0x43, 0xAF, 0x76,
        0xE3, 0xB3, 0x68, 0x66, 0x8E, 0x72, 0x66, 0x74, 0xF3, 0xE4, 0xDB, 0x36, 0x5D, 0x08, 0xD9, 0xD4
    };        

static DRAM_ATTR uint32_t cp = 0;             // Prover counter
static DRAM_ATTR uint8_t attest = 1;          // Attestation status

void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, int *mutexes, size_t num_sockets)
{
    // Parse attest_req
    uint32_t cv =
        (uint32_t)attest_req[SIMPLE_PLUS_ATTESTREQ_CV_OFFSET] |
        ((uint32_t)attest_req[SIMPLE_PLUS_ATTESTREQ_CV_OFFSET + 1] << 8) |
        ((uint32_t)attest_req[SIMPLE_PLUS_ATTESTREQ_CV_OFFSET + 2] << 16) |
        ((uint32_t)attest_req[SIMPLE_PLUS_ATTESTREQ_CV_OFFSET + 3] << 24);

    uint16_t vss_len =
        (uint16_t)attest_req[SIMPLE_PLUS_ATTESTREQ_VSSLEN_OFFSET] |
        ((uint16_t)attest_req[SIMPLE_PLUS_ATTESTREQ_VSSLEN_OFFSET + 1] << 8);

    size_t offset = SIMPLE_PLUS_ATTESTREQ_VSS_OFFSET;

    uint8_t *vss = malloc(vss_len);
    memcpy(vss, attest_req + offset, vss_len);
    offset += vss_len;

    uint8_t nonce[SIMPLE_PLUS_ATTESTREQ_NONCE_LEN];
    memcpy(nonce, attest_req + offset, SIMPLE_PLUS_ATTESTREQ_NONCE_LEN);
    offset += SIMPLE_PLUS_ATTESTREQ_NONCE_LEN;

    uint8_t h[SIMPLE_PLUS_ATTESTREQ_HMAC_LEN];
    memcpy(h, attest_req + offset, SIMPLE_PLUS_ATTESTREQ_HMAC_LEN);

    // Algorithm as per Figure 4 of SIMPLE paper
    if (cp < cv)
    {
        // Check received HMAC against locally computed HMAC
        uint8_t local_attest_req_hmac[SIMPLE_HMAC_LEN];
        Hacl_HMAC_compute_sha2_256(
            local_attest_req_hmac,
            k_auth,
            SIMPLE_KEY_SIZE,
            attest_req,
            SIMPLE_PLUS_ATTESTREQ_CV_LEN + SIMPLE_PLUS_ATTESTREQ_NONCE_LEN + SIMPLE_PLUS_ATTESTREQ_VSSLEN_LEN + vss_len
        );
        if (memcmp(local_attest_req_hmac, h, SIMPLE_HMAC_LEN) == 0)
        {
            // Broadcast to other nodes
            uint8_t *broadcast_buf = malloc(3 + attest_req_len);
            broadcast_buf[0] = CMD_SIMPLE_PLUS_ATTEST;
            broadcast_buf[1] = (uint8_t)attest_req_len;
            broadcast_buf[2] = (uint8_t)(attest_req_len >> 8);
            memcpy(broadcast_buf + 3, attest_req, attest_req_len);
            sa_protected_broadcast(broadcast_buf, 3 + attest_req_len, sockets, mutexes, num_sockets);
            ESP_LOGI(TAG_SIMPLE_PLUS, "[Attest] Broadcasted attest_req to %d other nodes", num_sockets);

            // Check if software state is valid
            uint8_t vss_prime[SIMPLE_HMAC_LEN];
            compute_software_state(k_attest, SIMPLE_KEY_SIZE, vss_prime);
            bool valid = false;
            for (int i = 0; i < vss_len * SIMPLE_HMAC_LEN; i += SIMPLE_HMAC_LEN)
            {
                if (memcmp(vss + i, vss_prime, SIMPLE_HMAC_LEN) == 0)
                {
                    valid = true;
                    break;
                }
            }
            
            if (valid == true)
            {
                attest = attest & 1;
                ESP_LOGI(TAG_SIMPLE_PLUS, "[Attest] Software state is valid (attest = %d)", attest);
            } 
            else
            {
                attest = attest & 0;
                ESP_LOGW(TAG_SIMPLE_PLUS, "[Attest] Software state is invalid (attest = %d)", attest);
            }

            // Update k_col
            uint32_t cloned_cv = cv;
            uint8_t hash_buf[SIMPLE_HMAC_LEN];
            while (cloned_cv - cp != 0)
            {
                memcpy(hash_buf, k_col, SIMPLE_HMAC_LEN);
                Hacl_Hash_SHA2_hash_256(hash_buf, SIMPLE_HMAC_LEN, k_col);
                cloned_cv = cloned_cv - 1;
            }

            // Update counter
            cp = cv;
        }
        else
        {
            ESP_LOGE(TAG_SIMPLE_PLUS, "[Attest] HMAC mismatch");
        }
    }
    else
    {
        ESP_LOGE(TAG_SIMPLE_PLUS, "[Attest] cp is greater than or equal to cv (%u >= %u)", cp, cv);
    }

    free(vss);
}