/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#include <algorithms.h>
#include "adaptertypes.h"
#include "ecumsg.h"

using namespace util;

const uint8_t* Ecumsg::refData_;

class EcumsgISO9141 : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
    virtual void addChecksum();
private:
    EcumsgISO9141() : Ecumsg(ISO9141) {
        __setHeader(0x68, 0x6A, 0xF1);
    }
};

class EcumsgISO14230 : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
    virtual void addChecksum();
    virtual uint8_t headerLength() const;
private:    
    EcumsgISO14230() : Ecumsg(ISO14230) {
        __setHeader(0xC0, 0x33, 0xF1);
    }
};

class EcumsgVPW : public Ecumsg {
    friend class Ecumsg;
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
    virtual void addChecksum();
private:    
    EcumsgVPW() : Ecumsg(VPW) {
        __setHeader(0x68, 0x6A, 0xF1);
    }
};

class EcumsgPWM : public Ecumsg {
    friend class Ecumsg;    
public:
    virtual void addHeaderAndChecksum();
    virtual bool stripHeaderAndChecksum();
    virtual void addChecksum();
private:
    EcumsgPWM() : Ecumsg(PWM) {
        __setHeader(0x61, 0x6A, 0xF1);
    }
};

/**
 * Factory method for adapter protocol messages
 * @param[in] type Message type
 * @return The message instance 
 */
Ecumsg* Ecumsg::instance(uint8_t type)
{
    Ecumsg* instance = nullptr;
    switch(type) {
        case ISO9141:
            instance = new EcumsgISO9141();
            break;
        case ISO14230:
            instance = new EcumsgISO14230();
            break;
        case VPW:
            instance = new EcumsgVPW();
            break;
        case PWM:
            instance = new EcumsgPWM();
            break;
    }
    
    if (instance) {
        // if header
        const ByteArray* bytes = AdapterConfig::instance()->getBytesProperty(PAR_HEADER_BYTES);
        if (bytes->length)
            instance->__setHeader(bytes->data);
    }
    return instance;
}

/**
 * Construct Ecumsg object
 */
Ecumsg::Ecumsg(uint8_t type) : data_(nullptr), type_(type), length_(0)
{
    data_ = localData_; // use "localData_" as "data_" until we change it
}

/**
 * Destructor
 */
Ecumsg::~Ecumsg()
{
}

/**
 * Get the string representation of message bytes
 * @param[out] str The output string
 */
void Ecumsg::sendReply() const
{
    const uint32_t OutLen = TX_BUFFER_LEN;
    static string str(OutLen * 3);
    uint32_t msgLen = length_;
    Spacer spacer(str);
    
    for (int n = 0; msgLen > 0; n += OutLen) {
        uint32_t len = min(msgLen, OutLen);
        to_ascii(&data_[n], len, str);
        msgLen -= len;
        if (msgLen > 0) {
            spacer.space();
        }
        AdptSendString(str);
        str.resize(0);
    }
    AdptSendReply(""); // line terminator
}

/**
 * Set the message data bytes, use external buffer to store
 * @param[in] data Data bytes
 * @param[in] length Data length
 */
void Ecumsg::setData(const uint8_t* data, uint32_t length)
{
    length_ = length;
    if (data != refData_) { // use internal buffer
        memcpy(localData_, data, min(length, static_cast<uint32_t>(sizeof(localData_))));
        data_ = localData_;
    }
    else { // use extrernal buffer
        data_ = const_cast<uint8_t*>(data);
    }
}

/**
 * Set header bytes
 * @param[in] header The header bytes
 */
void Ecumsg::__setHeader(uint8_t h1, uint8_t h2, uint8_t h3)
{
    header_[0] = h1;
    header_[1] = h2;
    header_[2] = h3;
    header_[HEADER_SIZE] = 0;
}

void Ecumsg::__setHeader(const uint8_t* header)
{
    memcpy(header_, header, HEADER_SIZE);
    header_[HEADER_SIZE] = 0;
}

/**
 * Adds the header to the message data bytes
 * @param[in] headerLen Header length
 */
void Ecumsg::__addHeader(uint32_t headerLen)
{
    // Shift data on headerLen to accommodate the header
    memmove(&data_[headerLen], data_, length_);
    memcpy(data_, header_, headerLen);
    length_ += headerLen;
}

/**
 * Remove the header from the message data bytes
 * @param[in] headerLen Header length
 */
void Ecumsg::__removeHeader(uint32_t headerLen)
{
    length_ -= headerLen;
    memmove(data_, &data_[headerLen], length_);
}

/**
 *  Adds the checksum to ISO 9141/14230 message
 */
void Ecumsg::__isoAddChecksum()
{
    uint8_t sum { 0 };
    for (int i = 0; i < length_; i++) {
        sum += data_[i];
    }
    data_[length_++] = sum;
}

/*
 * Add the checksum to J1850 message
 */
void Ecumsg::__j1850AddChecksum()
{
    const uint8_t* ptr = data_;
    int len = length_;
    uint8_t chksum = 0xFF;  // start with all one's

    while (len--) {
        int i = 8;
        uint8_t val = *(ptr++);
        while (i--) {
            if (((val ^ chksum) & 0x80) != 0) {
                chksum ^= 0x0E;
                chksum = (chksum << 1) | 1;
            }
            else {
                chksum = chksum << 1;
            }
            val = val << 1;
        }
    }
    data_[length_++] = ~chksum;
}

/**
 * Strip the checksum from the message
 */
void Ecumsg::__stripChecksum()
{
    length_--;
}

/**
 * Adds the header/checksum to ISO 9141 message
 */
void EcumsgISO9141::addHeaderAndChecksum()
{
    __addHeader(HEADER_SIZE);
    __isoAddChecksum();
}

/**
 * Adds the checksum to ISO 9141 message
 */
void EcumsgISO9141::addChecksum()
{
    __isoAddChecksum();
}

/**
 * Adds the header/checksum to ISO 14230 message
 */
void EcumsgISO14230::addHeaderAndChecksum()
{
    uint8_t headerForm = header_[0] >> 6;  // Table 1, 14230-2
    int headerSize = (headerForm == 0) ? 1 : 3;
    bool byteLenPresent = (length_ > 63) | ((header_[0] & 0x0F) == 0);
    if (byteLenPresent)
        headerSize++;
    
    uint8_t len = length_; // the message length without header
    
    // Shift data on headerSize to accommodate the header
    __addHeader(headerSize);
    
    // Figure out where to store len
    if (byteLenPresent) { // separate byte, the last of the header
        data_[headerSize - 1] = len;
        data_[0] &= 0xC0;
    }
    else { // the length is in the 1st byte, 63 bytes max
        data_[0] = (data_[0] & 0xC0) | len; 
    }
    
    __isoAddChecksum();
}

/**
 * Adds the checksum to ISO 14230 message
 */
void EcumsgISO14230::addChecksum()
{
    __isoAddChecksum();
}

/**
 * Strips the header/checksum from ISO 9141 message
 * @return true if header is valid, false otherwise
 */
bool EcumsgISO9141::stripHeaderAndChecksum()
{
    __removeHeader(HEADER_SIZE);
    __stripChecksum();
    return true;
}

uint8_t EcumsgISO14230::headerLength() const
{
    uint8_t headerForm = data_[0] >> 6;  // Table 1, 14230-2
    uint8_t formatLen = data_[0] & 0x3F; 
    uint8_t headerLen = (headerForm == 0) ? 1 : 3;
    if (formatLen == 0)
        headerLen++; // extra lenght byte is present
    return headerLen;
}

/**
 * Strips the header/checksum from ISO 14230 message
 * @return true if header is valid, false otherwise
 */
bool EcumsgISO14230::stripHeaderAndChecksum()
{
    __removeHeader(headerLength());
    __stripChecksum();
    return true;
}

/**
 * Adds the header/checksum to J1850 VPW message
 */
void EcumsgVPW::addHeaderAndChecksum()
{
    __addHeader(HEADER_SIZE);
    __j1850AddChecksum();
}

/**
 * Adds the checksum to J1850 VPW message
 */
void EcumsgVPW::addChecksum()
{
    __j1850AddChecksum();
}

/**
 * Strips the header/checksum from J1850 VPW message
 * @return true if header is valid, false otherwise
 */
bool EcumsgVPW::stripHeaderAndChecksum()
{
    __removeHeader(HEADER_SIZE);
    __stripChecksum();
    return true;
}

/**
 * Adds the header/checksum to J1850 PWM message
 */
void EcumsgPWM::addHeaderAndChecksum()
{
    __addHeader(HEADER_SIZE);
    __j1850AddChecksum();
}

/**
 * Adds the checksum to J1850 PWM message
 */
void EcumsgPWM::addChecksum()
{
    __j1850AddChecksum();
}

/**
 * Strips the header/checksum from J1850 PWM message
 * @return true if header is valid, false otherwise
 */
bool EcumsgPWM::stripHeaderAndChecksum()
{
    __removeHeader(HEADER_SIZE);
    __stripChecksum();
    return true;
}
