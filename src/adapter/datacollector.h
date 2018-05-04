/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __DATACOLLECTOR_H__ 
#define __DATACOLLECTOR_H__

#include <lstring.h>

class DataCollector {
public:
    DataCollector(uint32_t size, uint32_t reserved = 0);
    DataCollector& operator=(const DataCollector& collector);
    const util::string& getString() const {return str_; }
    const uint8_t* getData() const { return data_; }
    uint32_t getLength() const { return length_; }
    uint32_t getNumOfResponses() const;
    bool isData() const { return binary_; }
    bool isHugeBuffer() const;
    void putChar(char ch);
    void reset();
    
private:
    uint32_t limit_;
    util::string str_;
    uint8_t* data_;
    uint32_t length_;
    char previous_;
    bool binary_;
};

#endif //__DATACOLLECTOR_H__
