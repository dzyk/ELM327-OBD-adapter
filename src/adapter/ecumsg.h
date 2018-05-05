/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ECUMSG_H__ 
#define __ECUMSG_H__

#include <cstdint>
#include <lstring.h>
#include "adaptertypes.h"

using namespace std;

class Ecumsg {
public:
    const static uint8_t ISO9141  = 1;
    const static uint8_t ISO14230 = 2;
    const static uint8_t PWM      = 3;
    const static uint8_t VPW      = 4;
    const static int HEADER_SIZE  = 3;
    
    static Ecumsg* instance(uint8_t type);
    virtual ~Ecumsg();
    const uint8_t* data() const { return data_; }
    uint8_t* data() { return data_; }
    uint8_t type() const { return type_; }
    uint16_t length() const { return length_; }
    void length(uint16_t length) { length_ = length; }
    virtual void addHeaderAndChecksum() = 0;
    virtual void addChecksum() = 0;
    virtual bool stripHeaderAndChecksum() = 0;
    virtual uint8_t headerLength() const { return HEADER_SIZE; }
    Ecumsg& operator+=(uint8_t byte) { data_[length_++] = byte; return *this; }
    void setData(const uint8_t* data, uint32_t length);
    void sendReply() const;
    static void setReferenceData(const uint8_t* data) { refData_ = data; }
protected:
    Ecumsg(uint8_t type);
    void __setHeader(const uint8_t* header);
    void __setHeader(uint8_t h1, uint8_t h2, uint8_t h3);
    void __addHeader(uint32_t headerLen);
    void __removeHeader(uint32_t headerLen);
    void __isoAddChecksum();
    void __j1850AddChecksum();
    void __stripChecksum();
    
    uint8_t* data_;
    uint8_t type_;
    uint32_t length_; // message length
    uint8_t header_[HEADER_SIZE + 1];
    uint8_t localData_[OBD_OUT_MSG_LEN];
    static const uint8_t* refData_;
};

#endif //__ECUMSG_H__
