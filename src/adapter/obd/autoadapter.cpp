/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include "autoadapter.h"

void AutoAdapter::getDescription()
{
    AdptSendReply("AUTO");
}

void AutoAdapter::getDescriptionNum()
{
    AdptSendReply("0");
}

int AutoAdapter::onRequest(const uint8_t* data, int len)
{
    return REPLY_NO_DATA;
}

int AutoAdapter::doConnect(int protocol, bool sendReply)
{
    ProtocolAdapter* adapter = ProtocolAdapter::getAdapter(protocol);
    protocol = adapter->onConnectEcu(sendReply);
    if (protocol != 0) {
        setStatus(adapter->getStatus());
        return protocol;
    }
    return 0;
}

int AutoAdapter::onConnectEcu(bool sendReply)
{
    int protocol = 0;
    
    // PWM
    protocol = doConnect(ADPTR_PWM, sendReply);
    if (protocol > 0)
        return protocol;
    
    // VPW
    protocol = doConnect(ADPTR_VPW, sendReply);
    if (protocol > 0)
        return protocol;
    
    // ISO
    protocol = doConnect(ADPTR_ISO, sendReply);
    if (protocol > 0)
        return protocol;
    
    // CAN
    protocol = doConnect(ADPTR_CAN, sendReply);
    if (protocol > 0)
        return protocol;

    // CAN 29
    return doConnect(ADPTR_CAN_EXT, sendReply);
}

