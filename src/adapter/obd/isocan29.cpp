/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2017 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstdio>
#include <adaptertypes.h>
#include <CanDriver.h>
#include <led.h>
#include "canhistory.h"
#include "canmsgbuffer.h"
#include "isocan.h"
#include "obdprofile.h"
#include "timeoutmgr.h"

using namespace std;
using namespace util;

/**
 * Response for "ATDP"
 */
void IsoCan29Adapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, ISO 15765-4 (CAN 29/500)" : "ISO 15765-4 (CAN 29/500)");
}
/**
 * Response for "ATDPN"
 */
void IsoCan29Adapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A7" : "7"); 
}

/**
 * Get 29-bit CAN Id, use either default or custom one
 **/
uint32_t IsoCan29Adapter::getID() const
{ 
    IntAggregate id(0x18DB33F1); // Default Id for 29-bit CAN OBD
    
    // Get custom header bytes
    const ByteArray* hdr = config_->getBytesProperty(PAR_HEADER_BYTES);
    if (hdr->length) {
        id.lvalue = hdr->asCanId();
    }

    // Get the CAN priority byte
    const ByteArray* prioBits = config_->getBytesProperty(PAR_CAN_PRIORITY_BITS);
    if (prioBits->length) {
        id.bvalue[3] = prioBits->data[0] & 0x1F;
    }
    return id.lvalue;
}

/**
 * Set 29-bit CAN OBD filter and mask
 **/
void IsoCan29Adapter::setFilterAndMask() 
{
    uint32_t mask = 0x1FFFFF00; // Default mask for 29-bit CAN
    
    const ByteArray* maskBytes = config_->getBytesProperty(PAR_CAN_MASK);
    if (maskBytes->length) {
        mask = maskBytes->asCanId() & 0x1FFFFFFF;
    }
    
    uint32_t filter = 0x18DAF100; // Default filter for 29-bit CAN
    
    const ByteArray* filterBytes = config_->getBytesProperty(PAR_CAN_FILTER);
    if (filterBytes->length) {
        filter = filterBytes->asCanId() & 0x1FFFFFFF;
    }
    
    // Set mask and filter 29 bit
    driver_->setFilterAndMask(filter, mask, true);
}

/**
 * 29-bit CAN control frame handler implementation
 **/
void IsoCan29Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    CanMsgBuffer ctrlData(0x18DA00F1, true, 8, 0x30, 0x0, 0x00);
    ctrlData.id |= (msg->id & 0xFF) << 8;
    driver_->send(&ctrlData);
    
    // Message log
    history_->add2Buffer(&ctrlData, true, 0);
}

/**
 * Open the protocol, set CAN speed, filter and mask, LED control and adaptive timing
 **/
void IsoCan29Adapter::open()
{
    driver_->setSpeed(CanDriver::ISO15765_CAN_500K);
    
    setFilterAndMask();
    
    // Start using LED timer
    AdptLED::instance()->startTimer();
    
    // Reset adaptive timing
    TimeoutManager::instance()->reset();
}
