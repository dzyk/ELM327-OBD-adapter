/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __PWM_H__ 
#define __PWM_H__

#include "padapter.h"

class PwmDriver;
class Timer;

class PwmAdapter : public ProtocolAdapter {
public:
    friend class ProtocolAdapter;
    virtual int onRequest(const uint8_t* data, uint32_t len, uint32_t numOfResp);
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual void open();
    virtual void wiringCheck();
    virtual int onConnectEcu(bool sendReply);
    virtual int getProtocol() const { return PROT_J1850_PWM; }
private:
    PwmAdapter();
    bool waitForSof();
    int receiveByte(uint8_t& val);
    int getIfr();
    int sendToEcu(const Ecumsg* msg);
    int receiveFromEcu(Ecumsg* msg, int maxLen);
    uint32_t getP2MaxTimeout() const;
    int requestImpl(const uint8_t* data, uint32_t len, uint32_t numOfResp, bool sendReply);
    bool sendByte(uint8_t val);
    void sendSof();
    void sendIfr();
    Timer*     timer_;
    PwmDriver* driver_;
};

#endif //__PWM_H__
