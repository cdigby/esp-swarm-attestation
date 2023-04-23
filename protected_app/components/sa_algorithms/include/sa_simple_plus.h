#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "esp_attr.h"

#include "sa_shared.h"
#include "sa_protected_comms.h"

// Command codes from sa_comms.h
// Ideally, we would want some way of generating valid comms packets without needing information from the comms stack
// at compile time
#define CMD_SIMPLE_PLUS_ATTEST      0x07

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

void simple_plus_prover_attest(uint8_t *attest_req, size_t attest_req_len, int *sockets, int *mutexes, size_t num_sockets);