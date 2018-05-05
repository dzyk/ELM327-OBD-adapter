/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

#include <climits>
#include <cctype>
#include <memory>
#include <cstdlib>
#include <algorithms.h>
#include "adaptertypes.h"
#include "datacollector.h"


using namespace std;
using namespace util;

const int CollectorStrLen = 16;

DataCollector::DataCollector(uint32_t size, uint32_t reserved) 
  : str_(CollectorStrLen), 
    length_(0), 
    previous_(0), 
    binary_(true)
{
    data_ = new uint8_t[size + reserved]; // Heap alocation
    limit_ = size;
}

DataCollector& DataCollector::operator=(const DataCollector& collector)
{
    // Self assignment check
    if (this != &collector) {
        str_ = collector.str_;
        length_ = util::min(collector.length_, limit_);
        previous_ = collector.previous_;
        binary_ = collector.binary_;
        memcpy(data_, collector.data_, length_);
    }
    return *this;
}

void DataCollector::putChar(char ch)
{
    if (ch == ' ' || ch == 0 ) // Ignore spaces
        return;
    
    // Make it uppercase
    ch = toupper(ch);
    binary_ = binary_ ? isxdigit(ch) : false;
    
    if (str_.length() < CollectorStrLen) {
        str_ += ch;
    }
    if (length_ < limit_) { // If more then BINARY_DATA_LEN, ignore
        if (binary_ && previous_) {
            char twoChars[] = { previous_, ch, 0 }; 
            data_[length_++] = strtoul(twoChars, 0, 16);
            previous_ = 0;
        }
        else {
            previous_ = ch; // save for the next ops
        }
    }
}

void DataCollector::reset()
{
    str_.clear();
    length_   = 0;
    previous_ = 0;
    binary_   = true;
}

bool DataCollector::isHugeBuffer() const
{
    return length_ > OBD_IN_MSG_DLEN;
}
    
uint32_t DataCollector::getNumOfResponses() const
{    
    const uint32_t NumOfResp = 0xFFFFFFFF;
    
    if (previous_ != 0 && !isHugeBuffer()) {
        char twoChars[] = { previous_, 0 }; 
        uint32_t value  = strtoul(twoChars, 0, 16);
        return (value > 0) ? value : NumOfResp;
    }
    else {
        return NumOfResp;
    }
}
