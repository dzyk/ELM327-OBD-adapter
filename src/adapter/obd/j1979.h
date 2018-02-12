/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __J1979_DEFINES_H__
#define __J1979_DEFINES_H__

#include <cstdint>

using namespace std;

// SAE J1979 timeouts definition
enum IsoTimeouts {
    W1_MAX_TIMEOUT     =  300,
    W3_TIMEOUT         =  20,
    W4_MAX_TIMEOUT     =  50,
    P1_MAX_TIMEOUT     =  20,
    P2_MAX_TIMEOUT     =  50,
    P3_MIN_TIMEOUT     =  55,
    W4_TIMEOUT         =  33,
    P4_TIMEOUT         =  7,
    KEEP_ALIVE_MAX_NUM =  5, // Disconnect after 5 failed,
    DEFAULT_WAKEUP_TIME =  3000,
    P2_MAX_TIMEOUT_S    =  5000 // P2* timeout
};

const uint8_t TESTER_ADDRESS = 0xF1;

#endif //__J1979_DEFINES_H__
