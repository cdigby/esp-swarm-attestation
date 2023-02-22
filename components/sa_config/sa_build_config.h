#pragma once

// Change this to the ID of the node we want to build for
// 1 = A, 2 = B, etc.
#define NODE_ID 1

//////////////////////////////////////
#define NODE_PASSWORD "test1234"

#if NODE_ID == 1
    #define NODE_SSID "SA_DEMO_A"
    #define NODE_PARENT ""

#elif NODE_ID == 2
    #define NODE_SSID "SA_DEMO_B"
    #define NODE_PARENT "SA_DEMO_A"

#elif NODE_ID == 3
    #define NODE_SSID "SA_DEMO_C"
    #define NODE_PARENT "SA_DEMO_B"

#elif NODE_ID == 4
    #define NODE_SSID "SA_DEMO_D"
    #define NODE_PARENT "SA_DEMO_B"

#endif

