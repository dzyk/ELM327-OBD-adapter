/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2017 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __TIMEOUT_MGR_H__
#define __TIMEOUT_MGR_H__

#include <cstdint>

using namespace std;

// Adaptive Timing control
//
class TimeoutManager {
public:
    const static int AT0 = 0;
    const static int AT1 = 1;
    const static int AT2 = 2;

    static TimeoutManager* instance();
    void mode(int val) { mode_ = val; }
    void p2Timeout(uint32_t timeout);
    void reset() { timeout_ = 0, threshold_ = 0; }
    uint32_t p2Timeout() const;
    uint32_t at0Timeout() const;
private:
    TimeoutManager();
    uint32_t at1Timeout() const;
    uint32_t at2Timeout() const;
    bool multiplier() const;
    int mode_;
    uint32_t timeout_;
    uint32_t threshold_;
};

#endif
