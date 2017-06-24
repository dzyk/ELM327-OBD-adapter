/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2017 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __J1939_CONN_MGR_H__
#define __J1939_CONN_MGR_H__

#include <cstdint>
using namespace std;

class J1939Adapter;
class CanMsgBuffer;

class J1939ConnectionMgr {
public:
    const static uint32_t TP_CM_ACK_PGN  = 0xE800;
    const static uint32_t TP_CM_CTRL_PGN = 0xEC00;
    const static uint32_t TP_CM_DT_PGN   = 0xEB00;
    const static uint32_t TP_CM_RTS      = 0x10;
    const static uint32_t TP_CM_CTS      = 0x11;
    const static uint32_t TP_CM_ACK      = 0x13;

    J1939ConnectionMgr(J1939Adapter* adapter);
    void pgn(uint8_t pgn0, uint8_t pgn1, uint8_t pgn2);
    bool isValidAck(const CanMsgBuffer* msg) const;
    bool rts(const CanMsgBuffer* msg);
    bool data(const CanMsgBuffer* msg);
    uint32_t size() const { return size_; }
private:
    bool sendAck();

    J1939Adapter* adapter_;
    uint8_t  nFrames_;
    uint16_t size_;
    uint8_t  pgn_[3];
    uint8_t  src_;
    uint8_t  dst_;
    uint8_t  currNum_;
};

#endif //__CAN_CONN_MGR_H__
