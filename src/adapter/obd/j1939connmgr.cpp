/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#include <cstring>
#include <algorithms.h>
#include "j1939connmgr.h"
#include "isocan.h"
#include "canmsgbuffer.h"

using namespace std;
using namespace util;

/**
 * Construct J1939 Connection Manager
 */
J1939ConnectionMgr::J1939ConnectionMgr(J1939Adapter* adapter)
  : adapter_(adapter),
    nFrames_(0),
    size_(0),
    src_(0),
    dst_(0),
    currNum_(0)
{
    pgn_[0] = pgn_[1] = pgn_[2] = 0;
}

/**
 * Process Connection Mode Request to Send (TP.CM_RTS)
 * @param[in] msg CanMsgbuffer instance pointer
 * @return true if processed succesfully, false otherwise
 */ 
bool J1939ConnectionMgr::rts(const CanMsgBuffer* msg)
{
    size_    = to_int(msg->data[1], msg->data[2]);
    nFrames_ = msg->data[3];
    dst_     = lsb(msg->id);
    src_     = adapter_->getID() & 0xFF;
    currNum_ = 0;
    
    // Send TP.CM_CST frame
    const uint8_t startNum = 1;
    uint8_t cts[] = { TP_CM_CTS, nFrames_, startNum , 0xFF, 0xFF, pgn_[0], pgn_[1], pgn_[2] };
    uint32_t ctsId = 0x1C000000 | (TP_CM_CTRL_PGN << 8) | (dst_ << 8) | src_;
    return adapter_->sendFrameToEcu(cts, sizeof(cts), sizeof(cts), ctsId);
}

/**
 * Process Data Transfer Message (TP.DT)
 * @param[in] msg CanMsgbuffer instance pointer
 * @return true if processed succesfully, false otherwise
 */ 
bool J1939ConnectionMgr::data(const CanMsgBuffer* msg)
{
    if (msg->data[0] != ++currNum_) {
        // recovery
        return false;
    }
    else if (currNum_ == nFrames_) { // the last one, send ack
        return sendAck();
    }
    return true;
}
/**
 * Send End of Message Acknowledgment (TP.CM_EndOfMsgACK)
 * @return true if ACK message sent succeeded, false otherwise
 */
bool J1939ConnectionMgr::sendAck()
{
    uint8_t size0 = lsb(size_);
    uint8_t size1 = _2nd(size_);
    uint8_t ack[] = { TP_CM_ACK, size0, size1, nFrames_, 0xFF, pgn_[0], pgn_[1], pgn_[2] };
    uint32_t ackId = 0x1C000000 | (TP_CM_CTRL_PGN << 8) | (pgn_[0] << 8) | src_;
    return adapter_->sendFrameToEcu(ack, sizeof(ack), sizeof(ack), ackId);
}

/**
 * Validate received ACK message against the current PGN
 * @param[in] msg CanMsgbuffer instance pointer
 * @return true if ACK message related, false otherwise
 */
bool J1939ConnectionMgr::isValidAck(const CanMsgBuffer* msg) const
{
    return ((msg->data[5] == pgn_[0]) & (msg->data[6] == pgn_[1]) & (msg->data[7] == pgn_[2]));
}

/**
 * Set the current PGN function, refer to J1939-71 doc
 * @param[in] pgn0 LSB of parameter group number
 * @param[in] pgn1 2nd parameter group number byte
 * @param[in] pgn0 MSB of parametr group number
 */
void J1939ConnectionMgr::pgn(uint8_t pgn0, uint8_t pgn1, uint8_t pgn2)
{
    pgn_[0] = pgn0;
    pgn_[1] = pgn1;
    pgn_[2] = pgn2;
}
