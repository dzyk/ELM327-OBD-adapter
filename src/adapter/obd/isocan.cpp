/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <memory>
#include <cstdio>
#include <adaptertypes.h>
#include <Timer.h>
#include <candriver.h>
#include <led.h>
#include "canmsgbuffer.h"
#include <algorithms.h>
#include "obdprofile.h"
#include "j1979.h"
#include "isocan.h"
#include "canhistory.h"
#include "timeoutmgr.h"

using namespace std;
using namespace util;

const int CAN_FRAME_LEN = 8;

IsoCanAdapter::IsoCanAdapter()
{
    extended_   = false;
    driver_     = CanDriver::instance();
    history_    = new CanHistory();
    sts_        = REPLY_NO_DATA;
    canExtAddr_ = false;
}

/**
 * Send buffer to ECU using CAN
 * @param[in] data The message data bytes
 * @param[in] len The message length
 * @return true if OK, false if data issues
 */
bool IsoCanAdapter::sendToEcu(const uint8_t* buff, int length)
{
    uint8_t dlc = 0;
    uint8_t data[CAN_FRAME_LEN] = {0};
    
    if (length > 0x0FFF)
        return false; // The max CAN length

    // Extended address byte
    const ByteArray* canExt = config_->getBytesProperty(PAR_CAN_EXT);
    int totalLen = canExt->length ? (length + 2) : (length + 1); // + cea byte, + len byte
    canExtAddr_ = canExt->length; // set class instance member
    
    if (totalLen <= CAN_FRAME_LEN) { 
        int i = 0;
        if (canExt->length) { // CAN extended addressing
            data[i++] = canExt->data[0];
        }

        // OBD type format with dlc=8, and first byte = length
        //
        dlc = (getProtocol() != PROT_ISO15765_USR_B) ? CAN_FRAME_LEN : totalLen;
        data[i++] = length;
        memcpy(data + i, buff, length);
        return sendFrameToEcu(data, totalLen, dlc);
    }
    else {
        return false;
    }
}

/**
 * Send a single CAN frame to ECU
 * @param[in] data The message data bytes
 * @param[in] length The message length
 * @param[in] dlc The message DLC
 * @return true if OK, false if data issues
 */
bool IsoCanAdapter::sendFrameToEcu(const uint8_t* data, uint8_t length, uint8_t dlc)
{
    CanMsgBuffer msgBuffer(getID(), extended_, dlc, 0);
    memcpy(msgBuffer.data, data, length);
    
    // Message log
    history_->add2Buffer(&msgBuffer, true, 0);

    if (!driver_->send(&msgBuffer)) { 
        return false; // REPLY_DATA_ERROR
    }
    return true;
}

/**
 * Format reply for "H1" option
 * @param[in] msg CanMsgbuffer instance pointer
 * @param[out] str The output string
 * @param[in] dlen The data length flag
 */
void IsoCanAdapter::formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str, int dlen)
{
    CanIDToString(msg->id, str, msg->extended);
    bool useSpaces = AdapterConfig::instance()->getBoolProperty(PAR_SPACES);
    bool isDLC = AdapterConfig::instance()->getBoolProperty(PAR_CAN_DLC);
    if (useSpaces) {
        str += ' '; // number of chars + space
    }
    if (isDLC) {
        str += msg->dlc + '0'; // add DLC byte
        if (useSpaces) {
            str += ' ';
        }
    }
    to_ascii(msg->data, dlen, str);
}

/**
 * Process single frame
 * @param[in] msg CanMsgbuffer instance pointer
 */
void IsoCanAdapter::processFrame(const CanMsgBuffer* msg)
{
    util::string str;
    uint32_t offst = canExtAddr_ ? 2 : 1;
    uint32_t dlen = msg->data[offst - 1];
    
    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str, dlen + offst);
    }
    else {
        to_ascii(msg->data + offst, dlen, str);
    }
    AdptSendReply(str);
}

/**
 * Process first frame
 * @param[in] msg CanMsgbuffer instance pointer
 */
void IsoCanAdapter::processFirstFrame(const CanMsgBuffer* msg)
{
    util::string str;
    uint32_t offst = canExtAddr_ ? 2 : 1;
    uint32_t dlen = canExtAddr_ ? 5 : 6;
    uint32_t msgLen = (msg->data[offst - 1] & 0x0F) << 8 | msg->data[offst];

    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str, 8);
    }
    else {
        CanIDToString(msgLen, str, false); // we need only 3 digits
        AdptSendReply(str);
        str = "0: ";
        to_ascii(msg->data + offst + 1, dlen, str);
    }
    AdptSendReply(str);
}

/**
 * Process next frame
 * @param[in] msg CanMsgbuffer instance pointer
 * @param[in] n Frame sequential number
 */
void IsoCanAdapter::processNextFrame(const CanMsgBuffer* msg, int n)
{
    util::string str;
    uint32_t offst = canExtAddr_ ? 2 : 1;
    uint32_t dlen = canExtAddr_ ? 6 : 7;

    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str, 8);
    }
    else {
        char prefix[4]; // space for 1.5 bytes max
        sprintf(prefix, "%X: ", (n & 0x0F));
        str = prefix;
        to_ascii(msg->data + offst, dlen, str);
    }
    AdptSendReply(str);
}

/**
 * ISO14230 Timing Exceptions handler, requestCorrectlyReceived-ResponsePending
 * @param[in] msg CanMsgbuffer instance pointer
 * @return true if timeout exception, false otherwise
 */
bool IsoCanAdapter::checkResponsePending(const CanMsgBuffer* msg)
{
    int offst = canExtAddr_ ? 1 : 0;
    
    // Looking for "requestCorrectlyReceived-ResponsePending", 7F.XX.78
    if (msg->data[1 + offst] == 0x7F && msg->data[3 + offst] == 0x78) {
        return true;
    }
    return false;
}

/**
 * Receives a sequence of bytes from the CAN bus
 * @param[in] sendReply Send reply to user flag
 * @param[in] numOfResp The number of responces to wait for
 * @return true if message received, false otherwise
 */
bool IsoCanAdapter::receiveFromEcu(bool sendReply, uint32_t numOfResp)
{
    const int MAX_PEND_RESP_NUM = 100;
    int pendRespCounter = 0;
    const int p2Timeout = getP2MaxTimeout();
    CanMsgBuffer msgBuffer;
    bool msgReceived = false;
    canExtAddr_ = config_->getBytesProperty(PAR_CAN_EXT)->length; // set class instance member
    int frameNum = 0;
    
    Timer* timer = Timer::instance(0);
    timer->start(p2Timeout);

    uint32_t num = 0;
    do {
        if (!driver_->isReady())
            continue;
        driver_->read(&msgBuffer);
        
        // Measure the response time
        TimeoutManager::instance()->p2Timeout(timer->value());
        
        // Message log
        history_->add2Buffer(&msgBuffer, false, msgBuffer.msgnum);
        
        if (!checkResponsePending(&msgBuffer) || pendRespCounter > MAX_PEND_RESP_NUM) {
            // Reload the timer, regular P2 timeout
            timer->start(p2Timeout);
        }
        else {
            // Reload the timer, P2* timeout
            timer->start(P2_MAX_TIMEOUT_S);
            pendRespCounter++;
        }
        
        // Check the CAN receiver address
        /*
        if (canExt) {
            if (msgBuffer.data[0] != testerAddress)
                continue;
        }*/

        num++; // number of received messages
        msgReceived = true;
        if (!sendReply)
            continue;
        
        // CAN extextended address
        uint8_t keyByte = canExtAddr_ ? msgBuffer.data[1] : msgBuffer.data[0];
        switch ((keyByte & 0xF0) >> 4) {
            case CANSingleFrame:
                processFrame(&msgBuffer);
                break;
            case CANFirstFrame:
                processFlowFrame(&msgBuffer);
                processFirstFrame(&msgBuffer);
                break;
            case CANConsecutiveFrame:
                processNextFrame(&msgBuffer, ++frameNum);
                break;
            default:
                processFrame(&msgBuffer); // oops
        }
    } while (!timer->isExpired() && (num < numOfResp));

    return msgReceived;
}

/**
 * CAN control frame handler
 * @param[in] fs Frame FS parameter
 * @param[in] bs Frame BS parameter
 * @return true if message received, false otherwise
 */
bool IsoCanAdapter::receiveControlFrame(uint8_t& fs, uint8_t& bs, uint8_t& stmin)
{
    const int p2Timeout = getP2MaxTimeout();
    CanMsgBuffer msgBuffer;
    
    Timer* timer = Timer::instance(0);
    timer->start(p2Timeout);

    do {
        if (!driver_->isReady())
            continue;
        driver_->read(&msgBuffer);
        
        // Log Message
        history_->add2Buffer(&msgBuffer, false, msgBuffer.msgnum);

        if (!canExtAddr_ && (msgBuffer.data[0] & 0xF0) == 0x30) {
            fs = msgBuffer.data[0] & 0x0F;
            bs = msgBuffer.data[1];
            stmin = msgBuffer.data[2];
            return true;
        }
        else if (canExtAddr_ && (msgBuffer.data[1] & 0xF0) == 0x30) {
            fs = msgBuffer.data[1] & 0x0F;
            bs = msgBuffer.data[2];
            stmin = msgBuffer.data[3];
            return true;
        }
    } while (!timer->isExpired());

    return false;
}

/**
 * The P2 Max timeout to use, use either ISO default or custom value
 * @return The timeout value
 */
uint32_t IsoCanAdapter::getP2MaxTimeout() const
{
    return TimeoutManager::instance()->p2Timeout();
}

/**
 * Global entry ECU send/receive function
 * @param[in] data The message data bytes
 * @param[in] len The message length
 * @param[in] numOfResp The number of responces to wait for
 * @return The completion status code
 */
int IsoCanAdapter::onRequest(const uint8_t* data, uint32_t len, uint32_t numOfResp)
{
    if (!sendToEcu(data, len))
        return REPLY_DATA_ERROR;
    return receiveFromEcu(true, numOfResp) ? REPLY_NONE : REPLY_NO_DATA;
}

/**
 * Will try to send PID0 to query the CAN protocol
 * @param[in] sendReply Reply flag
 * @return Protocol value if ECU is supporting CAN protocol, 0 otherwise
 */
int IsoCanAdapter::onConnectEcu(bool sendReply)
{
    CanMsgBuffer msgBuffer(getID(), extended_, 8, 0x02, 0x01, 0x00);
    sts_ = REPLY_OK;
    open();

    if (!config_->getBoolProperty(PAR_BYPASS_INIT)) {
        bool retCode = driver_->send(&msgBuffer);
        
        // Message log
        history_->add2Buffer(&msgBuffer, true, 0);

        if (retCode) { 
            if (receiveFromEcu(sendReply, 0xFFFFFFFF)) {
                connected_ = true;
                return extended_ ? PROT_ISO15765_2950 : PROT_ISO15765_1150;
            }
        }
        close(); // Close only if not succeeded
        sts_ = REPLY_NO_DATA;
        return 0;
    }
    else {
        connected_ = true;
        return extended_ ? PROT_ISO15765_2950 : PROT_ISO15765_1150;
    }
}

/**
 * Print the messages buffer
 */
void IsoCanAdapter::dumpBuffer()
{
    history_->dumpCurrentBuffer();
}

/**
 * Test wiring connectivity for CAN
 */
void IsoCanAdapter::wiringCheck()
{
    bool failed = false;
    driver_->setBitBang(true);

    driver_->setBit(0);
    Delay1us(100);
    if (driver_->getBit() != 0) {
        AdptSendReply("CAN wiring failed [0->1]");
        failed = true;
    }        

    driver_->setBit(1);
    Delay1us(100);
    if (driver_->getBit() != 1) {
        AdptSendReply("CAN wiring failed [1->0]");
        failed = true;
    }    
    
    if (!failed) {
        AdptSendReply("CAN wiring is OK");
    }

    driver_->setBitBang(false);
}

/**
 * IsoCan11Adapter class members
 */
void IsoCan11Adapter::open()
{
    setFilterAndMask();
    
    //Start using LED timer
    AdptLED::instance()->startTimer();
    
    // Reset adaptive timing
    TimeoutManager::instance()->reset();
}

uint32_t IsoCan11Adapter::getID() const
{ 
    IntAggregate id;

    const ByteArray* hdr = config_->getBytesProperty(PAR_HEADER_BYTES);
    if (hdr->length) {
        id.lvalue = hdr->asCanId() & 0x7FF;
    }
    else {
        id.lvalue = 0x7DF;
    }
    return id.lvalue;
}

void IsoCan11Adapter::setFilterAndMask()
{
    // Mask 11 bit
    IntAggregate mask;
    const ByteArray* maskBytes = config_->getBytesProperty(PAR_CAN_MASK);
    if (maskBytes->length) {
        mask.lvalue = maskBytes->asCanId() & 0x7FF;
    }
    else { // Default mask for 11 bit CAN
        mask.lvalue = 0x7F8;
    }
    
    // Filter 11 bit
    IntAggregate filter;
    const ByteArray* filterBytes = config_->getBytesProperty(PAR_CAN_FILTER);
    if (filterBytes->length) {
        filter.lvalue = filterBytes->asCanId() & 0x7FF;
    }
    else { // Default filter for 11 bit CAN
        filter.lvalue = 0x7E8;
    }
    
    // Set mask and filter 11 bit
    driver_->setFilterAndMask(filter.lvalue, mask.lvalue, false);
}

void IsoCan11Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    if (!config_->getBoolProperty(PAR_CAN_FLOW_CONTROL))
        return; // ATCFC0
    
    int flowMode = config_->getIntProperty(PAR_CAN_FLOW_CTRL_MD);
    const ByteArray* hdr = config_->getBytesProperty(PAR_CAN_FLOW_CTRL_HDR);
    const ByteArray* bytes = config_->getBytesProperty(PAR_CAN_FLOW_CTRL_DAT);
    
    CanMsgBuffer ctrlData(getID(), false, 8, 0x30, 0x00, 0x00);
    ctrlData.id |= (msg->id & 0x07);
    if(flowMode == 1) {
        ctrlData.id = hdr->asCanId() & 0x7FF;
    }
    if(flowMode > 0) {
        memset(ctrlData.data, CanMsgBuffer::DefaultByte, sizeof(ctrlData.data));
        memcpy(ctrlData.data, bytes->data, bytes->length);
    }
    driver_->send(&ctrlData);
    
    // Message log
    history_->add2Buffer(&ctrlData, true, 0);
}

int IsoCan11Adapter::getProtocol() const
{ 
    return config_->getIntProperty(PAR_PROTOCOL);
}

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

void IsoCan11Adapter::setReceiveAddress(const util::string& par)
{
}

/**
 * IsoCan29Adapter class members
 */
void IsoCan29Adapter::open()
{
    setFilterAndMask();
    
    // Start using LED timer
    AdptLED::instance()->startTimer();
    
    // Reset adaptive timing
    TimeoutManager::instance()->reset();
}

uint32_t IsoCan29Adapter::getID() const
{ 
    IntAggregate id;
    uint8_t canPriority = 0;
    
    // Get the highest 29bit header byte
    const ByteArray* prioBits = config_->getBytesProperty(PAR_CAN_PRIORITY_BITS);
    if (prioBits->length) {
        canPriority = prioBits->data[0] & 0x1F;
    }
    else {
        canPriority = 0x18; // Default for OBD  
    }
    
    const ByteArray* hdr = config_->getBytesProperty(PAR_HEADER_BYTES);
    if (hdr->length) {
        id.lvalue = hdr->asCanId();
        id.bvalue[3] = canPriority;
    }
    else {
        id.lvalue = 0x18DB33F1;
    }
    return id.lvalue;
}

void IsoCan29Adapter::setFilterAndMask() 
{
    // Mask 29 bit
    IntAggregate mask;
    const ByteArray* maskBytes = config_->getBytesProperty(PAR_CAN_MASK);
    if (maskBytes->length) {
        mask.lvalue = maskBytes->asCanId() & 0x1FFFFFFF;
    }
    else { // Default mask for 29 bit CAN
        mask.lvalue = 0x1FFFFF00;
    }
    
    // Filter 29 bit
    IntAggregate filter;
    const ByteArray* filterBytes = config_->getBytesProperty(PAR_CAN_FILTER);
    if (filterBytes->length) {
        filter.lvalue = filterBytes->asCanId() & 0x1FFFFFFF;
    }
    else { // Default for 29 bit CAN
        filter.lvalue = 0x18DAF100;
    }
    
    // Set mask and filter 29 bit
    driver_->setFilterAndMask(filter.lvalue, mask.lvalue, true);
}

void IsoCan29Adapter::processFlowFrame(const CanMsgBuffer* msg)
{
    CanMsgBuffer ctrlData(getID(), true, 8, 0x30, 0x0, 0x00);
    ctrlData.id |= (msg->id & 0xFF) << 8;
    driver_->send(&ctrlData);
    
    // Message log
    history_->add2Buffer(&ctrlData, true, 0);
}

void IsoCan29Adapter::getDescription()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AUTO, ISO 15765-4 (CAN 29/500)" : "ISO 15765-4 (CAN 29/500)");
}

void IsoCan29Adapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "A7" : "7"); 
}

void IsoCan29Adapter::setReceiveAddress(const util::string& par)
{
}
