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

static int64_t get_time_ms()
{
    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    return (int64_t)(((int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec) / 1000);
}

void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, size_t num_sockets)
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
            // Broadcast attest_req to other nodes
            uint8_t *broadcast_buf = malloc(3 + attest_req_len);
            broadcast_buf[0] = CMD_SIMPLE_PLUS_ATTEST;
            broadcast_buf[1] = (uint8_t)attest_req_len;
            broadcast_buf[2] = (uint8_t)(attest_req_len >> 8);
            memcpy(broadcast_buf + 3, attest_req, attest_req_len);
            sa_protected_broadcast(broadcast_buf, 3 + attest_req_len, sockets, num_sockets);
            free(broadcast_buf);
            ESP_LOGI(TAG_SIMPLE_PLUS, "[attest] Broadcasted attest_req to %d other nodes", num_sockets);

            // Check if software state is valid
            uint8_t vss_prime[SIMPLE_HMAC_LEN];
            compute_software_state(k_attest, SIMPLE_KEY_SIZE, vss_prime);
            bool valid = false;
            for (int i = 0; i < vss_len; i += SIMPLE_HMAC_LEN)
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
                ESP_LOGI(TAG_SIMPLE_PLUS, "[attest] Software state is valid");
                if (attest == 0)
                {
                    ESP_LOGW(TAG_SIMPLE_PLUS, "[attest] Software state was invalid in the past", attest);
                }
            } 
            else
            {
                attest = attest & 0;
                ESP_LOGW(TAG_SIMPLE_PLUS, "[attest] Software state is invalid");
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
            ESP_LOGE(TAG_SIMPLE_PLUS, "[attest] HMAC mismatch");
        }
    }
    else
    {
        ESP_LOGE(TAG_SIMPLE_PLUS, "[attest] cp is greater than or equal to cv (%u >= %u)", cp, cv);
    }

    free(vss);
}

void simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int *sockets, size_t num_sockets)
{
    // Check that collect_req contains data and is not just an HMAC
    if (collect_req_len != SIMPLE_PLUS_COLLECTREQ_LEN)
    {
        ESP_LOGE(TAG_SIMPLE_PLUS, "[collect] Invalid collect_req length (%d)", collect_req_len);
        return;
    }

    // Parse collect_req
    uint16_t timeout =
        (uint16_t)collect_req[SIMPLE_PLUS_COLLECTREQ_TIMEOUT_OFFSET] |
        ((uint16_t)collect_req[SIMPLE_PLUS_COLLECTREQ_TIMEOUT_OFFSET + 1] << 8);

    uint8_t h[SIMPLE_PLUS_COLLECTREQ_HMAC_LEN];
    memcpy(h, collect_req + SIMPLE_PLUS_COLLECTREQ_HMAC_OFFSET, SIMPLE_PLUS_ATTESTREQ_HMAC_LEN);

    // Algorithm as per Figure 5 of SIMPLE paper
    // Check received HMAC against locally computed HMAC
    uint8_t local_collect_req_hmac[SIMPLE_HMAC_LEN];
    Hacl_HMAC_compute_sha2_256(
        local_collect_req_hmac,
        k_col,
        SIMPLE_HMAC_LEN,
        collect_req,
        collect_req_len - SIMPLE_HMAC_LEN
    );

    if (memcmp(local_collect_req_hmac, h, SIMPLE_HMAC_LEN) == 0)
    {
        // Send ACK to sender
        uint8_t ack_buf[1 + SIMPLE_HMAC_LEN];
        ack_buf[0] = CMD_SIMPLE_PLUS_COLLECT_ACK;

        Hacl_HMAC_compute_sha2_256(
            ack_buf + 1,
            k_col,
            SIMPLE_HMAC_LEN,
            ack_buf,
            1
        );

        sa_protected_send(sender_sock, ack_buf, 1 + SIMPLE_HMAC_LEN);
        ESP_LOGI(TAG_SIMPLE_PLUS, "[collect] ACK sent");

        // Broadcast collect_req to other nodes
        uint8_t *broadcast_buf = malloc(3 + collect_req_len);
        broadcast_buf[0] = CMD_SIMPLE_PLUS_COLLECT;
        broadcast_buf[1] = (uint8_t)collect_req_len;
        broadcast_buf[2] = (uint8_t)(collect_req_len >> 8);
        memcpy(broadcast_buf + 3, collect_req, collect_req_len);
        sa_protected_broadcast(broadcast_buf, 3 + collect_req_len, sockets, num_sockets);
        free(broadcast_buf);
        ESP_LOGI(TAG_SIMPLE_PLUS, "[collect] Broadcasted collect_req to %d other nodes", num_sockets);

        // Wait for ACKs from other nodes, will wait until timeout or until all connected nodes have sent an ACK
        uint8_t rx_buf[1 + SIMPLE_HMAC_LEN];
        memset(rx_buf, 0, 1 + SIMPLE_HMAC_LEN);

        // Use this array to flag as true sockets that have received an ACK
        // sockets[i] corresponds to got_ack[i]
        // If we try and recv from a socket that already sent an ack, we may accidentally discard a report
        int num_acks = 0;
        bool *got_ack = malloc(num_sockets);
        for (int i = 0; i < num_sockets; i++)
        {
            got_ack[i] = false;
        }

        int64_t start_time = get_time_ms();
        while (get_time_ms() - start_time < timeout)
        {
            for (int i = 0; i < num_sockets; i++)
            {
                // Skip sockets that already sent an ack
                if (got_ack[i] == true)
                {
                    continue;
                }

                // Try and receive an ACK
                if (sa_protected_recv(sockets[i], rx_buf, 1 + SIMPLE_HMAC_LEN) != 1 + SIMPLE_HMAC_LEN)
                {
                    continue;
                }

                if (rx_buf[0] == CMD_SIMPLE_PLUS_COLLECT_ACK)
                {
                    // Check HMAC
                    uint8_t local_ack_hmac[SIMPLE_HMAC_LEN];
                    Hacl_HMAC_compute_sha2_256(
                        local_ack_hmac,
                        k_col,
                        SIMPLE_HMAC_LEN,
                        rx_buf,
                        1
                    );

                    if (memcmp(local_ack_hmac, rx_buf + 1, SIMPLE_HMAC_LEN) != 0)
                    {
                        ESP_LOGE(TAG_SIMPLE_PLUS, "[collect] HMAC mismatch getting ACK from socket %d", i);
                        continue;
                    }

                    got_ack[i] = true;
                    num_acks++;
                }
            }

            if (num_acks >= num_sockets)
            {
                break;
            }
        }
        ESP_LOGI(TAG_SIMPLE_PLUS, "[collect] ACKs received: %d/%d", num_acks, num_sockets);

        // Generate this node's attestation report
        size_t aggregated_report_len = ((NODE_ID - 1) / 8) + 1;    // We need 1 bit per node
        uint8_t *aggregated_report = malloc(aggregated_report_len);
        memset(aggregated_report, 0, aggregated_report_len);
        if (attest == 1)
        {
            // Set the (NODE_ID - 1)th bit of local_report by left shifting the last byte
            // We subtract 1 from NODE_ID as NODE_ID starts at 1         
            aggregated_report[aggregated_report_len - 1] = 0x01 << ((NODE_ID - 1) % 8);
        }

        // Aggregate other reports
        memset(rx_buf, 0, 1 + SIMPLE_HMAC_LEN);

        // As before, we use got_report to flag sockets that have sent a report
        int num_reports = 0;
        bool *got_report = malloc(num_acks);
        for (int i = 0; i < num_acks; i++)
        {
            got_report[i] = false;
        }

        start_time = get_time_ms();
        while (get_time_ms() - start_time < timeout)
        {
            for (int i = 0; i < num_sockets; i++)
            {
                // Skip sockets that didn't send ACK
                if (got_ack[i] == false)
                {
                    continue;
                }

                // Skip sockets that already sent a report
                if (got_report[i] == true)
                {
                    continue;
                }

                // Get command code
                if (sa_protected_recv(sockets[i], rx_buf, 1) != 1)
                {
                    continue;
                }

                if (rx_buf[0] != CMD_SIMPLE_PLUS_COLLECT_REPORT)
                {
                    continue;
                }

                // Get report length
                if (sa_protected_recv(sockets[i], rx_buf, 2) != 2)
                {
                    continue;
                }

                uint16_t report_len = (uint16_t)rx_buf[0] | ((uint16_t)rx_buf[1] << 8);
                uint8_t *report_buf = malloc(3 + report_len + SIMPLE_HMAC_LEN);
                report_buf[0] = CMD_SIMPLE_PLUS_COLLECT_REPORT;
                report_buf[1] = rx_buf[0];
                report_buf[2] = rx_buf[1];

                // Get report
                if (sa_protected_recv(sockets[i], report_buf + 3, report_len + SIMPLE_HMAC_LEN) != report_len + SIMPLE_HMAC_LEN)
                {
                    free(report_buf);
                    continue;
                }

                // Check HMAC
                uint8_t local_report_hmac[SIMPLE_HMAC_LEN];
                Hacl_HMAC_compute_sha2_256(
                    local_report_hmac,
                    k_col,
                    SIMPLE_HMAC_LEN,
                    report_buf,
                    3 + report_len
                );

                if (memcmp(local_report_hmac, report_buf + 3 + report_len, SIMPLE_HMAC_LEN) != 0)
                {
                    ESP_LOGE(TAG_SIMPLE_PLUS, "[collect] HMAC mismatch getting report from socket %d", i);
                    free(report_buf);
                    continue;
                }

                // If received report is longer than our report, we need to allocate more memory
                if (report_len > aggregated_report_len)
                {
                    aggregated_report = realloc(aggregated_report, report_len);
                    aggregated_report_len = report_len;
                }

                // Aggregate reports
                for (int i = 0; i < report_len; i++)
                {
                    aggregated_report[i] |= report_buf[3 + i];
                }

                num_reports++;
                free(report_buf);
            }

            if (num_reports >= num_acks)
            {
                break;
            }
        }
        free(got_ack);
        free(got_report);
        ESP_LOGI(TAG_SIMPLE_PLUS, "[collect] Reports received: %d/%d", num_reports, num_acks);

        // Send aggregated report
        uint8_t *tx_buf = malloc(3 + aggregated_report_len + SIMPLE_HMAC_LEN);
        tx_buf[0] = CMD_SIMPLE_PLUS_COLLECT_REPORT;
        tx_buf[1] = (uint8_t)aggregated_report_len;
        tx_buf[2] = (uint8_t)(aggregated_report_len >> 8);
        memcpy(tx_buf + 3, aggregated_report, aggregated_report_len);

        Hacl_HMAC_compute_sha2_256(
            tx_buf + 3 + aggregated_report_len,
            k_col,
            SIMPLE_HMAC_LEN,
            tx_buf,
            3 + aggregated_report_len
        );

        sa_protected_send(sender_sock, tx_buf, 3 + aggregated_report_len + SIMPLE_HMAC_LEN);
        free(tx_buf);
        ESP_LOGI(TAG_SIMPLE_PLUS, "Aggregated report sent");

        free(aggregated_report);
    }
    else
    {
        ESP_LOGE(TAG_SIMPLE_PLUS, "[Collect] HMAC mismatch");
    }
}