//
// IsoCanAdapter structures and definitions
//

#ifndef __ISO_CAN_H__
#define __ISO_CAN_H__

#include "padapter.h"


class CanDriver;
class CanHistory;
struct CanMsgBuffer;

class IsoCanAdapter : public ProtocolAdapter {
public:
    static const int CANSingleFrame      = 0;
    static const int CANFirstFrame       = 1;
    static const int CANConsecutiveFrame = 2;
    static const int CANFlowControlFrame = 3;
public:
    virtual int onRequest(const uint8_t* data, int len);
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
    bool receiveFromEcu(bool sendReply);
    bool checkResponsePending(const CanMsgBuffer* msg, bool canExt);
    void processFrame(const CanMsgBuffer* msg);
    void formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str);
    bool receiveControlFrame(uint8_t& fs, uint8_t& bs, uint8_t& stmin);
    int getP2MaxTimeout() const;
protected:
    CanDriver*  driver_;
    CanHistory* history_;
    bool        extended_;
};

class IsoCan11Adapter : public IsoCanAdapter {
public:
    IsoCan11Adapter() {}
    virtual void getDescription();
    virtual void getDescriptionNum();
    virtual uint32_t getID() const;
    virtual void setFilterAndMask();
    virtual void processFlowFrame(const CanMsgBuffer* msgBuffer);
    virtual int getProtocol() const;
    virtual void open();
    static void setReceiveAddress(const util::string& par);
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
    static void setReceiveAddress(const util::string& par);
};

#endif //__ISO_CAN_H__
