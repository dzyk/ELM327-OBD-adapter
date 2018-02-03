/**
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009-2018 ObdDiag.Net. All rights reserved.
 *
 */

//
// Lightweight string class, Version: 3.21
//

#ifndef __LSTRING_H__ 
#define __LSTRING_H__

#include <cstdint>

using namespace std;

namespace util {

class string {
public:
    typedef char* iterator;
    typedef const char* const_iterator;

    const static uint32_t STRING_SIZE = 50;
    const static uint32_t npos = 0xFFFFFFFF;
    string(uint32_t size = STRING_SIZE);
    string(const char* s);
    string(const string& str);
    string(uint32_t count, char ch);
    string(string&& str) noexcept;
    ~string();
    void resize(uint32_t count);
    void resize(uint32_t count, char ch);
    string& append(const char* s);
    string& append(const char* s, uint32_t count);
    string& append(uint32_t count, char ch);
    string& assign(uint32_t count, char ch);
    void clear() noexcept;
    uint32_t copy(char* dest, uint32_t count, uint32_t pos = 0) const;
    const char* c_str() const noexcept { return data_; }
    bool empty() const noexcept { return (length_ == 0); }
    uint32_t find(const string& str, uint32_t pos = 0) const noexcept;
    uint32_t find(char ch, uint32_t pos = 0) const noexcept;
    uint32_t length() const noexcept { return length_; }
    string substr(uint32_t pos, uint32_t count = npos) const;
    string& operator+=(const string& other);
    string& operator+=(const char* s);
    string& operator+=(char ch);
    char operator[](uint32_t pos) const { return data_[pos]; }
    char& operator[](uint32_t pos) { return data_[pos]; }
    string& operator=(const string& str);
    string& operator=(string&& str) noexcept;
    string& operator=(const char* s);
    void reserve(uint32_t size);
    iterator begin() noexcept { return data_; }
    iterator end() noexcept { return begin() + length_; }
    const_iterator begin() const noexcept { return data_; }
    const_iterator end() const noexcept { return begin() + length_; }
private:
    void __move_assign(string& str) noexcept;
    void __init(uint32_t size) noexcept;
    char* data_;
    uint16_t length_;
    uint16_t allocatedLength_;
};

bool operator==(const string& lhs, const char* rhs);
bool operator!=(const string& lhs, const char* rhs);
bool operator==(const string& lhs, const string& rhs);
string operator+(const string& lhs, const string& rhs);
string operator+(const char* lhs, const string& rhs);
string operator+(char ch, const string& rhs);
string operator+(const string& lhs, const char* rhs);
string operator+(const string& lhs, char ch);


} // namespace util

#endif //__LSTRING_H__


