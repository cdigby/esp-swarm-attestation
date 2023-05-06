#pragma once

// Change this to the ID of the node we want to build for
// 1 = A, 2 = B, etc.
#define NODE_ID 1

// Node ids need to start from 1 (instead of 0) for IP address generation, or verifier will not be able to connect

// To change a node's broadcasted name, change NODE_SSID
// To change a node's parent name, change NODE_PARENT
// To change value that a node's fake app memory is initialised to, change NODE_FAKE_MEMORY_CONTENTS
//      This will affect the attestation result, and will likely require modification to the verifier software.

//////////////////////////////////////
#define NODE_PASSWORD                   "test1234"      // Password for connecting to node's access point
#define NODE_FAKE_MEMORY_SIZE           (64 * 1024)     // Size of the fake app memory, changing this will require changes to the verifiers

#if NODE_ID == 1
    #define NODE_SSID                   "SA_DEMO_1"
    #define NODE_PARENT                 ""
    #define NODE_FAKE_MEMORY_CONTENTS   0x01

#elif NODE_ID == 2
    #define NODE_SSID                   "SA_DEMO_2"
    #define NODE_PARENT                 "SA_DEMO_1"
    #define NODE_FAKE_MEMORY_CONTENTS   0x02

#elif NODE_ID == 3
    #define NODE_SSID                   "SA_DEMO_3"
    #define NODE_PARENT                 "SA_DEMO_2"
    #define NODE_FAKE_MEMORY_CONTENTS   0x03

#elif NODE_ID == 4
    #define NODE_SSID                   "SA_DEMO_4"
    #define NODE_PARENT                 "SA_DEMO_2"
    #define NODE_FAKE_MEMORY_CONTENTS   0x04

#endif

