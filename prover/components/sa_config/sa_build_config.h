#pragma once

// Change this to the ID of the node we want to build for
// Node IDs must start from 1 (instead of 0) for IP address generation, or verifier will not be able to connect
#define NODE_ID 1

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
    #define NODE_PARENT                 "SA_DEMO_1"
    #define NODE_FAKE_MEMORY_CONTENTS   0x03

#elif NODE_ID == 4
    #define NODE_SSID                   "SA_DEMO_4"
    #define NODE_PARENT                 "SA_DEMO_1"
    #define NODE_FAKE_MEMORY_CONTENTS   0x04

#elif NODE_ID == 5
    #define NODE_SSID                   "SA_DEMO_5"
    #define NODE_PARENT                 "SA_DEMO_2"
    #define NODE_FAKE_MEMORY_CONTENTS   0x05

#elif NODE_ID == 6
    #define NODE_SSID                   "SA_DEMO_6"
    #define NODE_PARENT                 "SA_DEMO_2"
    #define NODE_FAKE_MEMORY_CONTENTS   0x06

#elif NODE_ID == 7
    #define NODE_SSID                   "SA_DEMO_7"
    #define NODE_PARENT                 "SA_DEMO_3"
    #define NODE_FAKE_MEMORY_CONTENTS   0x07

#elif NODE_ID == 8
    #define NODE_SSID                   "SA_DEMO_8"
    #define NODE_PARENT                 "SA_DEMO_3"
    #define NODE_FAKE_MEMORY_CONTENTS   0x08

#elif NODE_ID == 9
    #define NODE_SSID                   "SA_DEMO_9"
    #define NODE_PARENT                 "SA_DEMO_4"
    #define NODE_FAKE_MEMORY_CONTENTS   0x09

#elif NODE_ID == 10
    #define NODE_SSID                   "SA_DEMO_10"
    #define NODE_PARENT                 "SA_DEMO_4"
    #define NODE_FAKE_MEMORY_CONTENTS   0x0A

#endif

