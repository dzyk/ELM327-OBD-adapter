/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#include <algorithms.h>
#include "adaptertypes.h"
#include "timeoutmgr.h"
#include "obd/obdprofile.h"

const uint32_t AT1_VALUE  = 30;
const uint32_t AT2_VALUE  = 10;
const uint32_t THRESHOLD  =  2;
const uint32_t DEFAULT_TIMEOUT = 200; // Default setting for ELM327

using namespace util;

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
    
#ifdef DEBUG_TM_VAL
    uint8_t data[2];
    util::string str;
    data[0] = val & 0xFF;
    data[1] = timeout_ & 0xFF;
    to_ascii(data, 2, str);
    AdptSendReply2(str);
#endif
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
#ifdef DEBUG_TM_VAL2
    uint8_t data[2];
    util::string str;
    data[0] = timeout & 0xFF;
    to_ascii(data, 1, str);
    AdptSendReply2(str);
#endif
    return timeout;
}


/**
 *  Get the timeout for AT0 mode
 */
uint32_t TimeoutManager::at0Timeout() const
{
    uint32_t p2Timeout = AdapterConfig::instance()->getIntProperty(PAR_TIMEOUT); 
    uint32_t timeoutMultVal = multiplier() ? 5 : 1;
    return p2Timeout ? (p2Timeout * 4 * timeoutMultVal) : DEFAULT_TIMEOUT; 
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

/**
 *  Get the timeout multiplier for CAN/J1939
 */
bool TimeoutManager::multiplier() const
{
    switch (OBDProfile::instance()->getProtocol()) {
        case PROT_ISO15765_1150:
        case PROT_ISO15765_2950:
        case PROT_ISO15765_1125:
        case PROT_ISO15765_2925:
        case PROT_ISO15765_USR_B:
            return AdapterConfig::instance()->getBoolProperty(PAR_CAN_TIMEOUT_MLT);
    }
    return false;
}
