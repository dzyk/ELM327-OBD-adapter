/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2017 ObdDiag.Net. All rights reserved.
 *
 */

#include <adaptertypes.h>
#include "timeoutmgr.h"
#include <lstring.h>

const uint32_t AT1_VALUE  = 30;
const uint32_t AT2_VALUE  = 10;
const uint32_t THRESHOLD  =  2;
const uint32_t DEFAULT_TIMEOUT = 200; // Default setting for ELM327

template <class T> const T& max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

template <class T> const T& min(const T& a, const T& b) {
    return (a < b) ? a : b;
}

/**
 * Construct the TimeoutManager object
 */
TimeoutManager::TimeoutManager() : mode_(AT1), timeout_(0), threshold_(0)
{
}

/**
 * TimeoutManager singleton
 * @return TimeoutManager pointer
 */
TimeoutManager* TimeoutManager::instance()
{
    static TimeoutManager manager;
    return &manager;
}

/**
 *  Set P2 timeout
 */
void TimeoutManager::p2Timeout(uint32_t val)
{
    // Skip the first few timeouts, like ISO9141 slow init responses
    if (threshold_ < THRESHOLD) {
        threshold_++;
    }
    else {
        timeout_ = min(max(timeout_, val), at0Timeout());
    }
}
	
/**
 *  Get P2 timeout
 */
uint32_t TimeoutManager::p2Timeout() const
{
    if (timeout_ == 0) {
        return at0Timeout(); // The very first time
    }
    uint32_t timeout = 0;    
	switch (mode_) {
		case AT1:
		    timeout = at1Timeout();
            break;
		case AT2:
		    timeout = at2Timeout();
            break;
		case AT0:
		default:
			timeout = at0Timeout();
    }
    return timeout;
}

/**
 *  Get the timeout for AT0 mode
 */
uint32_t TimeoutManager::at0Timeout() const
{
    int p2Timeout = AdapterConfig::instance()->getIntProperty(PAR_TIMEOUT); 
    return p2Timeout ? (p2Timeout * 4) : DEFAULT_TIMEOUT; 
}

/**
 *  Get the timeout for AT1 mode
 */
uint32_t TimeoutManager::at1Timeout() const
{
    return timeout_ + AT1_VALUE;
}

/**
 *  Get the timeout for AT0 mode
 */
uint32_t TimeoutManager::at2Timeout() const
{
    return timeout_ + AT2_VALUE;
}
