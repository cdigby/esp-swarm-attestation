#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <sys/time.h>

#include "esp_attr.h"

#include "sa_build_config.h"
#include "sa_shared.h"
#include "sa_protected_comms.h"

// Command codes from sa_comms.h
// Ideally, we would want some way of generating valid comms packets without needing information from the comms stack
// at compile time
#define CMD_SIMPLE_PLUS_ATTEST      0x07
#define CMD_SIMPLE_PLUS_COLLECT     0x08

// Command codes internal to SIMPLE+
#define CMD_SIMPLE_PLUS_COLLECT_ACK         0xF0
#define CMD_SIMPLE_PLUS_COLLECT_REPORT      0xF1

// Structure of SIMPLE+ AttestReq
// Since we are reading this from a uint8_t array sent over the network by a python script, it is simpler
// to use byte offsets rather than a struct
#define SIMPLE_PLUS_ATTESTREQ_CV_LEN        4
#define SIMPLE_PLUS_ATTESTREQ_VSSLEN_LEN    2
// VSS goes here and is variable length (= vss_len)
#define SIMPLE_PLUS_ATTESTREQ_NONCE_LEN     32
#define SIMPLE_PLUS_ATTESTREQ_HMAC_LEN      SIMPLE_HMAC_LEN

#define SIMPLE_PLUS_ATTESTREQ_CV_OFFSET     0
#define SIMPLE_PLUS_ATTESTREQ_VSSLEN_OFFSET (SIMPLE_PLUS_ATTESTREQ_CV_OFFSET + SIMPLE_PLUS_ATTESTREQ_CV_LEN)
#define SIMPLE_PLUS_ATTESTREQ_VSS_OFFSET    (SIMPLE_PLUS_ATTESTREQ_VSSLEN_OFFSET + SIMPLE_PLUS_ATTESTREQ_VSSLEN_LEN)

// Structure of SIMPLE+ collect_req
#define SIMPLE_PLUS_COLLECTREQ_TIMEOUT_LEN  2
#define SIMPLE_PLUS_COLLECTREQ_HMAC_LEN     SIMPLE_HMAC_LEN
#define SIMPLE_PLUS_COLLECTREQ_LEN          (SIMPLE_PLUS_COLLECTREQ_TIMEOUT_LEN + SIMPLE_PLUS_COLLECTREQ_HMAC_LEN)

#define SIMPLE_PLUS_COLLECTREQ_TIMEOUT_OFFSET 0
#define SIMPLE_PLUS_COLLECTREQ_HMAC_OFFSET  (SIMPLE_PLUS_COLLECTREQ_TIMEOUT_OFFSET + SIMPLE_PLUS_COLLECTREQ_TIMEOUT_LEN)

// SIMPLE+ report
#define SIMPLE_PLUS_INITIAL_REPORT_LEN    ((NODE_ID / 8) + 1)    // We need 1 bit per node

void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, size_t num_sockets);
void simple_plus_prover_collect(uint8_t *collect_req, size_t collect_req_len, int sender_sock, int *sockets, size_t num_sockets);