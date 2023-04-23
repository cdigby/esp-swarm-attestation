#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_attr.h"
#include "esp_log.h"

#include "sa_shared.h"
#include "sa_protected_comms.h"

// Structure of SIMPLE message
// Since we are reading this from a uint8_t array sent over the network by a python script, it is simpler
// to use byte offsets rather than a struct
#define SIMPLE_MSG_CV_LEN           4
#define SIMPLE_MSG_VS_LEN           32
#define SIMPLE_MSG_NONCE_LEN        32
#define SIMPLE_MSG_HMAC_LEN         SIMPLE_HMAC_LEN
#define SIMPLE_MSG_LEN              (SIMPLE_MSG_CV_LEN + SIMPLE_MSG_VS_LEN + SIMPLE_MSG_NONCE_LEN + SIMPLE_MSG_HMAC_LEN)

#define SIMPLE_MSG_CV_OFFSET        0
#define SIMPLE_MSG_VS_OFFSET        (SIMPLE_MSG_CV_OFFSET + SIMPLE_MSG_CV_LEN)
#define SIMPLE_MSG_NONCE_OFFSET     (SIMPLE_MSG_VS_OFFSET + SIMPLE_MSG_VS_LEN)
#define SIMPLE_MSG_HMAC_OFFSET      (SIMPLE_MSG_NONCE_OFFSET + SIMPLE_MSG_NONCE_LEN)


// Structure of SIMPLE report
#define SIMPLE_REPORT_VALUE_LEN     1
#define SIMPLE_REPORT_HMAC_LEN      SIMPLE_HMAC_LEN
#define SIMPLE_REPORT_LEN           (SIMPLE_REPORT_VALUE_LEN + SIMPLE_REPORT_HMAC_LEN)

#define SIMPLE_REPORT_VALUE_OFFSET  0
#define SIMPLE_REPORT_HMAC_OFFSET   (SIMPLE_REPORT_VALUE_OFFSET + SIMPLE_REPORT_VALUE_LEN)


// Structure of SIMPLE HMAC data
// This is the data that the HMAC sent with the report is calculated over
#define SIMPLE_HMAC_DATA_VALUE_LEN  1
#define SIMPLE_HMAC_DATA_CP_LEN     4
#define SIMPLE_HMAC_DATA_NONCE_LEN  32
#define SIMPLE_HMAC_DATA_LEN        (SIMPLE_HMAC_DATA_VALUE_LEN + SIMPLE_HMAC_DATA_CP_LEN + SIMPLE_HMAC_DATA_NONCE_LEN)

#define SIMPLE_HMAC_DATA_VALUE_OFFSET  0
#define SIMPLE_HMAC_DATA_CP_OFFSET    (SIMPLE_HMAC_DATA_VALUE_OFFSET + SIMPLE_HMAC_DATA_VALUE_LEN)
#define SIMPLE_HMAC_DATA_NONCE_OFFSET (SIMPLE_HMAC_DATA_CP_OFFSET + SIMPLE_HMAC_DATA_CP_LEN)

void simple_prover(uint8_t *msg, size_t msg_len, int response_sock, int response_sock_mutex);