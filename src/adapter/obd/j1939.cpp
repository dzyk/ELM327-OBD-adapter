/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstdio>
#include <adaptertypes.h>
#include <algorithms.h>
#include <Timer.h>
#include <CmdUart.h>
#include <CanDriver.h>
#include <led.h>
#include "canhistory.h"
#include "canmsgbuffer.h"
#include "isocan.h"
#include "obdprofile.h"
#include "timeoutmgr.h"
#include "j1939connmgr.h"

using namespace std;
using namespace util;

const uint32_t J1939_MAX_TIMEOUT = 1500;

/**
 * Construct J1939 Adapter class with the connection manager helper
 */
J1939Adapter::J1939Adapter()
{ 
    extended_ = true; 
    mgr_      = new J1939ConnectionMgr(this);
}

/**
 * Response for "ATDP"
 */
void J1939Adapter::getDescription()
{
    AdptSendReply("SAE J1939 (CAN 29/250)");
}

/**
 * Response for "ATDPN"
 */
void J1939Adapter::getDescriptionNum()
{
    bool useAutoSP = config_->getBoolProperty(PAR_USE_AUTO_SP);
    AdptSendReply(useAutoSP ? "AA" : "A"); 
}

/**
 * Get 29-bit J1939 CAN Id, use either default or custom one
 **/
uint32_t J1939Adapter::getID() const
{ 
    IntAggregate id(0x18EAFFF9); // Default for J1939

    // Custom header bytes
    const ByteArray* hdr = config_->getBytesProperty(PAR_HEADER_BYTES);
    if (hdr->length) {
        id.lvalue = hdr->asCanId();
    }
    
    // Get CAN priority byte
    const ByteArray* prioByte = config_->getBytesProperty(PAR_CAN_PRIORITY_BITS);
    if (prioByte->length) {
        id.bvalue[3] = prioByte->data[0] & 0x1F;
    }
    
    // Tester address
    const ByteArray* ta = config_->getBytesProperty(PAR_TESTER_ADDRESS);
    if (ta->length) {
        id.bvalue[0] = ta->data[0];
    }
    return id.lvalue;
}

/**
 * Open the protocol, set J1939 CAN speed, LED control
 **/
void J1939Adapter::open()
{
    driver_->setSpeed(CanDriver::J1939_CAN_250K);
    
    // Start using LED timer
    AdptLED::instance()->startTimer();
}

/**
 * Global entry ECU send/receive function
 * @param[in] data The message data bytes
 * @param[in] len The message length
 * @param[in] numOfResp The number of responces to wait for, use 0xFFFFFFFF if not provided
 * @return The completion status code
 */
int J1939Adapter::onRequest(const uint8_t* data, uint32_t len, uint32_t numOfResp)
{
    uint8_t buff[CAN_FRAME_LEN] = {0};
    memcpy(buff, data, len);
    
    // Reorder bytes if "JS" is not set
    if (!config_->getBoolProperty(PAR_J1939_FMT)) {
        ReverseBytes(buff, len);
    }

    // Clear anything we might received before
    driver_->clearData();
    
    if (!sendToEcu(buff, len))
        return REPLY_DATA_ERROR;
    
    // Configure mask and filter based on the request destination address
    setFilterAndMaskForPGN(to_int(buff[0], buff[1], buff[2]));
    int sts = receiveFromEcu() ? REPLY_NONE : REPLY_NO_DATA;
    
    // Clear the mastks & filters
    driver_->clearFilters();
    return sts;
}

/**
 * Nothing here, just mark it as connected
 * @param[in] sendReply Protocol number
 * @return Protocol value if ECU is supporting CAN protocol, 0 otherwise
 */
int J1939Adapter::onConnectEcu(bool sendReply)
{
    open();
    sts_ = REPLY_OK;
    connected_ = true;
    return PROT_J1939;
}

/**
 * Send buffer to ECU using CAN
 * @param[in] buff The message data bytes
 * @param[in] length The message length
 * @return true if OK, false if data issues
 */
bool J1939Adapter::sendToEcu(const uint8_t* buff, int length)
{
    if (length > CAN_FRAME_LEN)
        return false;
    
    return sendFrameToEcu(buff, length, length);
}

/**
 * Receives a sequence of bytes from the CAN bus
 * @return true if message received, false otherwise
 */
uint32_t J1939Adapter::receiveFromEcu()
{
    const uint32_t timeout = getTimeout();
    CanMsgBuffer msgBuffer;
    uint32_t msgNum = 0;
    
    Timer* timer = Timer::instance(0);
    timer->start(timeout);
    
    do {
        if (!driver_->isReady())
            continue;
        if (!driver_->read(&msgBuffer))
            continue;
        
        // Message log
        history_->add2Buffer(&msgBuffer, false, msgBuffer.msgnum);
        
        // Reload the timer with timeout
        timer->start(timeout);
        
        switch ((msgBuffer.id & 0xFF0000) >> 8) {
            case J1939ConnectionMgr::TP_CM_ACK_PGN:
                if (!mgr_->isValidAck(&msgBuffer))
                    continue;
                processFrame(&msgBuffer);
                break; 
                
            case J1939ConnectionMgr::TP_CM_CTRL_PGN:
                mgr_->rts(&msgBuffer);
                processRtsFrame(&msgBuffer);
                break;
            
            case J1939ConnectionMgr::TP_CM_DT_PGN:
                mgr_->data(&msgBuffer);
                processDtFrame(&msgBuffer);
                break;
            
            default:
               processFrame(&msgBuffer);
        }
        msgNum++;
        
    } while (!timer->isExpired());

    return msgNum;
}

/**
 * Set CAN filter & mask to receive a response from particular PGN
 * @param[in] msg CanMsgbuffer instance pointer
 */
void J1939Adapter::setFilterAndMaskForPGN(uint32_t pgn)
{
    // PGN response mask/filter
    uint32_t mask = 0x00FFFF00; // mask for PDU format/specific
    uint32_t filter = (pgn << 8);
    driver_->setFilterAndMask(filter, mask, true, 1);
    
    // ACK response
    mask = 0x00FF0000;     // use only PDU format
    filter = (0xE8 << 16); // ACK PDU byte
    driver_->setFilterAndMask(filter, mask, true, 2);
    
    // RTS response
    mask = 0x00FFFF00;     // Use only PGN byte
    filter = (0xEC << 16) | ((pgn & 0xFF) << 8); // TP.CM_RTS PDU
    driver_->setFilterAndMask(filter, mask, true, 3);
    
    filter = (0xEB << 16) | ((pgn & 0xFF) << 8); // TP.DT PDU
    driver_->setFilterAndMask(filter, mask, true, 4);
}

/**
 * Process a single frame
 * @param[in] msg CanMsgbuffer instance pointer
 */
void J1939Adapter::processFrame(const CanMsgBuffer* msg)
{
    util::string str;
    
    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str);
    }
    else {
        to_ascii(msg->data, msg->dlc, str);
    }
    AdptSendReply(str);
}

/**
 * Process TP.CM_RTS control frame, start of multiframe transfer
 * @param[in] msg CanMsgbuffer instance pointer
 */
void J1939Adapter::processRtsFrame(const CanMsgBuffer* msg)
{
    const int STR_LEN = 4;
    char slen[STR_LEN];
    sprintf(slen, "%.3X", static_cast<unsigned>(mgr_->size()));
    AdptSendReply(slen);
}

/**
 * Process TP.DT data frame as part of multiframe transfer
 * @param[in] msg CanMsgbuffer instance pointer
 */
void J1939Adapter::processDtFrame(const CanMsgBuffer* msg)
{
    util::string str;
    
    if (config_->getBoolProperty(PAR_HEADER_SHOW)) {
        formatReplyWithHeader(msg, str);
    }
    else {
        const int STR_LEN = 5;
        char prefix[STR_LEN];
        sprintf(prefix, "%.2X: ", static_cast<unsigned>(msg->data[0]));
        str = prefix;
        to_ascii(msg->data + 1, msg->dlc - 1, str);
    }
    AdptSendReply(str);
}

/**
 * Format reply for "H1/JHF" options
 * @param[in] msg CanMsgbuffer instance pointer
 * @param[out] str The output string
 */
void J1939Adapter::formatReplyWithHeader(const CanMsgBuffer* msg, util::string& str)
{
    Spacer spacer(str);
    bool jhf0 = AdapterConfig::instance()->getBoolProperty(PAR_J1939_HEADER);
    
    if (jhf0) {
        // CAN Priority byte
        uint8_t priority = (msg->id & 0x1C000000) >> 26;
        str += to_ascii(priority);
        spacer.space();
        
        // J1939 PGN
        uint32_t pgn = (msg->id & 0x03FFFF00) >> 8;
        str += to_ascii((pgn & 0x30000) >> 16);
        str += to_ascii((pgn & 0x0F000) >> 12);
        str += to_ascii((pgn & 0x00F00) >> 8);
        str += to_ascii((pgn & 0x000F0) >> 4);
        str += to_ascii(pgn & 0x0000F);
        spacer.space();
        
        // Source address
        uint8_t sa = msg->id & 0x000000FF;
        str += to_ascii((sa & 0xF0) >> 4);
        str += to_ascii(sa & 0x0F);
    }
    else {
        CanIDToString(msg->id, str, true);
    }
    spacer.space();
    
    // The frame data
    to_ascii(msg->data, msg->dlc, str);
}

/**
 * Timeout to use
 * @return The timeout value
 */
uint32_t J1939Adapter::getTimeout() const
{
    uint32_t p2Timeout = AdapterConfig::instance()->getIntProperty(PAR_TIMEOUT); 
    bool timeoutMult = AdapterConfig::instance()->getBoolProperty(PAR_J1939_TIMEOUT_MLT);
    uint32_t timeoutMultVal = timeoutMult ? 5 : 1;
    return p2Timeout ? (p2Timeout * 4 * timeoutMultVal) : J1939_MAX_TIMEOUT; 
}

/**
 * Implementation of "ATDM1" command
 */
void J1939Adapter::monitor()
{
    monitorImpl(0xFECA, 0xFFFFFFFF);
}

/**
 * Implementation of "ATMP" command
 * @param[in] data The message data bytes
 * @param[in] numOfResp The number of responces to wait for
 */
void J1939Adapter::monitor(const uint8_t* data, uint32_t len, uint32_t numOfResp)
{
    monitorImpl(to_int(data[2], data[1], data[0]), numOfResp);
}

/**
 * Actual implementation of monitor
 */
void J1939Adapter::monitorImpl(uint32_t pgn, uint32_t numOfResp)
{
    bool silent = AdapterConfig::instance()->getBoolProperty(PAR_CAN_SILENT_MODE);
    if (silent) // Set silent mode
        driver_->setSilent(true);
    
    setFilterAndMaskForPGN(pgn);
    CmdUart* uart = CmdUart::instance();
    uart->monitor(true);
    uint32_t num = 0;
    do {
        num += receiveFromEcu();
    } while(!uart->isMonitorExit() && num < numOfResp);
    
    driver_->clearFilters();
    if (silent) { // Restore if set
        driver_->setSilent(false);
    }
    if (uart->isMonitorExit()) {
        AdptSendReply("STOPPED");
    }
    uart->monitor(false);
}
