#include "sa_simple.h"

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

static DRAM_ATTR uint32_t cp = 0;             // Prover counter

void simple_prover(uint8_t msg[SIMPLE_MSG_LEN], uint8_t h[SIMPLE_HMAC_LEN], int response_sock)
{
    // Parse msg
    uint32_t cv =
        (uint32_t)msg[SIMPLE_MSG_CV_OFFSET] |
        ((uint32_t)msg[SIMPLE_MSG_CV_OFFSET + 1] << 8) |
        ((uint32_t)msg[SIMPLE_MSG_CV_OFFSET + 2] << 16) |
        ((uint32_t)msg[SIMPLE_MSG_CV_OFFSET + 3] << 24);
        
    uint8_t vs[SIMPLE_MSG_VS_LEN];
    uint8_t nonce[SIMPLE_MSG_NONCE_LEN];
    memcpy(vs, msg + SIMPLE_MSG_VS_OFFSET, SIMPLE_MSG_VS_LEN);
    memcpy(nonce, msg + SIMPLE_MSG_NONCE_OFFSET, SIMPLE_MSG_NONCE_LEN);

    // Algorithm as per Figure 2 of SIMPLE paper
    if (cp < cv)
    {
        // Check received HMAC against locally computed HMAC
        uint8_t hmac_msg[SIMPLE_HMAC_LEN];
        Hacl_HMAC_compute_sha2_256(hmac_msg, k_auth, SIMPLE_KEY_SIZE, msg, SIMPLE_MSG_LEN);
        if (memcmp(hmac_msg, h, SIMPLE_HMAC_LEN) == 0)
        {
            // Increment counter
            cp = cv;

            // Start preparing report
            uint8_t report[SIMPLE_REPORT_LEN];
            uint8_t report_hmac_data_buf[SIMPLE_HMAC_DATA_LEN];

            report_hmac_data_buf[SIMPLE_HMAC_DATA_CP_OFFSET] = (uint8_t)cp;
            report_hmac_data_buf[SIMPLE_HMAC_DATA_CP_OFFSET + 1] = (uint8_t)(cp >> 8);
            report_hmac_data_buf[SIMPLE_HMAC_DATA_CP_OFFSET + 2] = (uint8_t)(cp >> 16);
            report_hmac_data_buf[SIMPLE_HMAC_DATA_CP_OFFSET + 3] = (uint8_t)(cp >> 24);

            memcpy(report_hmac_data_buf + SIMPLE_HMAC_DATA_NONCE_OFFSET, nonce, SIMPLE_HMAC_DATA_NONCE_LEN);

            // Check if software state is valid, finish report
            uint8_t vs_prime[SIMPLE_MSG_VS_LEN];
            compute_software_state(k_attest, SIMPLE_KEY_SIZE, vs_prime);
            if (memcmp(vs, vs_prime, SIMPLE_MSG_VS_LEN) == 0)
            {
                report[SIMPLE_REPORT_VALUE_OFFSET] = 1;
                report_hmac_data_buf[SIMPLE_HMAC_DATA_VALUE_OFFSET] = 1;
                Hacl_HMAC_compute_sha2_256(report + SIMPLE_REPORT_HMAC_OFFSET, k_auth, SIMPLE_KEY_SIZE, report_hmac_data_buf, SIMPLE_HMAC_DATA_LEN);
            }
            else
            {
                report[SIMPLE_REPORT_VALUE_OFFSET] = 0;
                report_hmac_data_buf[SIMPLE_HMAC_DATA_VALUE_OFFSET] = 0;
                Hacl_HMAC_compute_sha2_256(report + SIMPLE_REPORT_HMAC_OFFSET, k_auth, SIMPLE_KEY_SIZE, report_hmac_data_buf, SIMPLE_HMAC_DATA_LEN);
            }

            // Send report
            sa_protected_send(response_sock, report, SIMPLE_REPORT_LEN);
        }
    }
}