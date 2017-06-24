/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __CAND_RIVER_H__
#define __CAND_RIVER_H__

#include <cstdint>

using namespace std;

typedef void *CAN_HANDLE_T;
struct CanMsgBuffer;

class CanDriver {
public:
    const static int J1939_CAN_250K    = 0;
    const static int ISO15765_CAN_500K = 1;

    static CanDriver* instance();
    static void configure();
    void setSpeed(int speed);
    bool send(const CanMsgBuffer* buff);
    bool setFilterAndMask(uint32_t filter, uint32_t mask, bool extended);
    bool setFilterAndMask(uint32_t filter, uint32_t mask, bool extended, int num);
    bool isReady() const;
    bool read(CanMsgBuffer* buff);
    bool wakeUp();
    bool sleep();
    void setBitBang(bool val);
    void setBit(uint32_t val);
    void clearFilters();
    void clearData();
    void setSilent(bool val);
    uint32_t getBit();
    static CAN_HANDLE_T handle_;
private:
    CanDriver();
    void configRxMsgobj(uint32_t filter, uint32_t mask, uint8_t msgobj, bool can29bit, bool fifoLast);
    int speed_;
};

#endif //__CAN_DRIVER_H__
