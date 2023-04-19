// These definitions are needed by attestation algorithms, syscalls and network stack

//// SIMPLE ////
#define SIMPLE_KEY_SIZE     32
#define SIMPLE_HMAC_LEN     32

// Structure of SIMPLE message
// Since we are reading this from a uint8_t array sent over the network by a python script, it is simpler
// to use byte offsets rather than a struct
#define SIMPLE_MSG_CV_LEN           4
#define SIMPLE_MSG_VS_LEN           32
#define SIMPLE_MSG_NONCE_LEN        32
#define SIMPLE_MSG_LEN              (SIMPLE_MSG_CV_LEN + SIMPLE_MSG_VS_LEN + SIMPLE_MSG_NONCE_LEN)  
