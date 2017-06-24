/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2016 ObdDiag.Net. All rights reserved.
 *
 */

#ifndef __ALGORITHMS_H__ 
#define __ALGORITHMS_H__

#include "lstring.h"

namespace util {
    
    void to_lower(string& str);
    void to_upper(string& str);
    void remove_space(string& str);
    uint32_t stoul(const string& str, uint32_t* pos = 0, int base = 10);
    bool is_xdigits(const string& str);
    char to_ascii(uint8_t byte);
    template <class T> uint8_t lsb(T v) { return static_cast<uint8_t>(v); }
    template <class T> uint8_t _2nd(T v) { return (v & 0xFF00) >> 8; } 
    inline uint32_t to_int(uint8_t b0, uint8_t b1) { return (b1 << 8) | b0; }
    inline uint32_t to_int(uint8_t b0, uint8_t b1, uint8_t b2) { return (b2 << 16) | (b1 << 8) | b0; }
    inline uint32_t to_int(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) { return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0; }
    template <class T> const T& max(const T& a, const T& b) { return (a < b) ? b : a; }
    template <class T> const T& min(const T& a, const T& b) { return (a < b) ? a : b; }

}

#endif //__ALGORITHMS_H__
