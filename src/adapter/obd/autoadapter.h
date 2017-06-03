/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __AUTO_PROFILE_H__ 
#define __AUTO_PROFILE_H__

#include "padapter.h"

class AutoAdapter : public ProtocolAdapter {
public:
    AutoAdapter() { connected_ = false; }
    virtual int onConnectEcu(bool sendReply);
    virtual int onRequest(const uint8_t* data, uint32_t len, uint32_t numOfRes);
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual int getProtocol() const { return PROT_AUTO; }
    virtual void wiringCheck() {}
private:
    int doConnect(int protocol, bool sendReply);
};

#endif //__AUTO_PROFILE_H__
