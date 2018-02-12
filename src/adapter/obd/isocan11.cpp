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
void IsoCan11Adapter::getDescription()
{
    const char* desc = nullptr;
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    int protocol = getProtocol();
    
    if (protocol == PROT_ISO15765_USR_B) {
        desc = "USER1 (CAN 11/500)";
    }
    else {
        desc = useAutoSP ? "AUTO, ISO 15765-4 (CAN 11/500)" : "ISO 15765-4 (CAN 11/500)";
    }
    AdptSendReply(desc);
}

/**
 * Response for "ATDPN"
 */
void IsoCan11Adapter::getDescriptionNum()
{
    const char* desc = nullptr;
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    int protocol = getProtocol();
    
    if (protocol == PROT_ISO15765_USR_B) {
        desc = "B";
    }
    else {
        desc = useAutoSP ? "A6" : "6";
    }
    AdptSendReply(desc); 
}

/**
 * Get 11-bit CAN Id, use either default or custom one
 **/
uint32_t IsoCan11Adapter::getID() const
{ 
    uint32_t id = 0x7DF; // Default Id for 11-bit CAN OBD

    const ByteArray* hdr = config_->getBytesProperty(PAR_HEADER_BYTES);
    if (hdr->length) {
        id = hdr->asCanId() & 0x7FF;
    }
    return id;
}

/**
 * Set 11-bit CAN OBD filter and mask
 **/
void IsoCan11Adapter::setFilterAndMask()
{
    uint32_t mask = 0x7F8; // Default mask for 11-bit OBD CAN
    
    // Custom mask
    const ByteArray* maskBytes = config_->getBytesProperty(PAR_CAN_MASK);
    if (maskBytes->length) {
        mask = maskBytes->asCanId() & 0x7FF;
    }
    
    uint32_t filter = 0x7E8; // Default filter for 11-bit OBD CAN
    
    // Custom filter
    const ByteArray* filterBytes = config_->getBytesProperty(PAR_CAN_FILTER);
    if (filterBytes->length) {
        filter = filterBytes->asCanId() & 0x7FF;
    }
    
    // Set mask and filter 11 bit
    driver_->setFilterAndMask(filter, mask, false);
}

/**
 * 11-bit CAN control frame handler implementation
 **/
void IsoCan11Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    if (!config_->getBoolProperty(PAR_CAN_FLOW_CONTROL))
        return; // ATCFC0
    
    int flowMode = config_->getIntProperty(PAR_CAN_FLOW_CTRL_MD);
    const ByteArray* hdr = config_->getBytesProperty(PAR_CAN_FLOW_CTRL_HDR);
    const ByteArray* bytes = config_->getBytesProperty(PAR_CAN_FLOW_CTRL_DAT);
    
    CanMsgBuffer ctrlData(0x7E0, false, 8, 0x30, 0x00, 0x00);
    ctrlData.id |= (msg->id & 0x07);
    if(flowMode == 1 && hdr != nullptr) {
        ctrlData.id = hdr->asCanId() & 0x7FF;
    }
    if(flowMode > 0 && bytes != nullptr) {
        memset(ctrlData.data, CanMsgBuffer::DefaultByte, sizeof(ctrlData.data));
        memcpy(ctrlData.data, bytes->data, bytes->length);
    }
    driver_->send(&ctrlData);
    
    // Message log
    history_->add2Buffer(&ctrlData, true, 0);
}

/**
 * Open the protocol, set CAN speed, filter and mask, LED control and adaptive timing
 **/
void IsoCan11Adapter::open()
{
    driver_->setSpeed(CanDriver::ISO15765_CAN_500K);
    
    setFilterAndMask();
    
    //Start using LED timer
    AdptLED::instance()->startTimer();
    
    // Reset adaptive timing
    TimeoutManager::instance()->reset();
}
