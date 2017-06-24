/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2017 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ISO_CAN_H__
#define __ISO_CAN_H__

#include "padapter.h"


class CanDriver;
class CanHistory;
struct CanMsgBuffer;

const int CAN_FRAME_LEN = 8;

class IsoCanAdapter : public ProtocolAdapter {
public:
    static const int CANSingleFrame      = 0;
    static const int CANFirstFrame       = 1;
    static const int CANConsecutiveFrame = 2;
    static const int CANFlowControlFrame = 3;
public:
    virtual int onRequest(const uint8_t* data, uint32_t len, uint32_t numOfResp);
    virtual int onConnectEcu(bool sendReply);
    virtual void setCanCAF(bool val) {}
    virtual void wiringCheck();
    virtual void dumpBuffer();
protected:
    IsoCanAdapter();
    virtual uint32_t getID() const = 0;
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer) = 0;
    bool sendToEcu(const uint8_t* data, int len);
    bool sendFrameToEcu(const uint8_t* data, uint8_t len, uint8_t dlc);
    bool sendFrameToEcu(const uint8_t* data, uint8_t length, uint8_t dlc, uint32_t id);
    bool receiveFromEcu(bool sendReply, uint32_t numOfResp);
    bool checkResponsePending(const CanMsgBuffer* msg);
    void processFrame(const CanMsgBuffer* msg);
    void processFirstFrame(const CanMsgBuffer* msg);
    void processNextFrame(const CanMsgBuffer* msg, int n);
    void formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str, int dlen);
    bool receiveControlFrame(uint8_t& fs, uint8_t& bs, uint8_t& stmin);
    uint32_t getP2MaxTimeout() const;
protected:
    CanDriver*  driver_;
    CanHistory* history_;
    bool        extended_;
    bool        canExtAddr_;
};

class IsoCan11Adapter : public IsoCanAdapter {
public:
    IsoCan11Adapter() {}
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void setFilterAndMask();
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer);
    virtual int getProtocol() const { return PROT_ISO15765_1150; }
    virtual void open();
};

class IsoCan29Adapter : public IsoCanAdapter {
public:
    IsoCan29Adapter() { extended_ = true; }
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void setFilterAndMask();
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer);
    virtual int getProtocol() const { return PROT_ISO15765_2950; }
    virtual void open();
};

class J1939ConnectionMgr;
class J1939Adapter : public IsoCanAdapter {
public:
    friend class J1939ConnectionMgr;
    J1939Adapter();
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer) {}
    virtual int getProtocol() const { return PROT_J1939; }
    virtual void open();
    virtual int onRequest(const uint8_t* data, uint32_t len, uint32_t numOfResp);
    virtual int onConnectEcu(bool sendReply);
    virtual void monitor();
    virtual void monitor(const uint8_t* data, uint32_t len, uint32_t numOfResp);
private:
    bool sendToEcu(const uint8_t* data, int len);
    uint32_t receiveFromEcu();
    void setFilterAndMaskForPGN(uint32_t pgn);
    void processFrame(const CanMsgBuffer* msg);
    void processRtsFrame(const CanMsgBuffer* msg);
    void processDtFrame(const CanMsgBuffer* msg);
    void formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str);
    void monitorImpl(uint32_t pgn, uint32_t numOfResp);
    uint32_t getTimeout() const;
    J1939ConnectionMgr* mgr_;
};

#endif //__ISO_CAN_H__
